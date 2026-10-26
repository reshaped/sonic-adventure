/* collision records, registration and the checks each step runs */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "collision/polygon.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "player/player.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/util.h"

/* a sweep tests a player's shape at points this far apart, from where the
 * player was a step before to where it is now */
#define SWEEP_STRIDE 2.5F

/* the checks list a target near a player within 100 of it. this is 100
 * squared, for the squared distance list_near_and_test_reach measures */
#define NEAR_REACH (100.0F * 100.0F)

/* a shape lands on a rectangle when its bottom lies at most LAND_DEPTH
 * below the top. the rectangle pushes it out of a side only when its
 * bottom lies deeper */
#define LAND_DEPTH 4.0F

/* an end of a cylinder2 counts as a top the sphere stands on when it faces
 * within about 49 degrees of straight up. END_UP is the cosine it must
 * pass */
#define END_UP 0.65F

/* a player's shape less than WALL_REACH in front of a wall's face is
 * pushed back from it. the push is WALL_PUSH of how far the shape lies
 * inside that reach. a shape that crosses the face is put back in front of
 * it, while its center lies less than WALL_REACH behind */
#define WALL_REACH 50.0F
#define WALL_PUSH 0.06F

static void rectangle_push_out(struct collision_work *collision,
                               NJS_POINT3 *pos, struct collision_work *other,
                               NJS_POINT3 *center);

/* a sphere 20 across: two poles and nine rings of ten points */
static NJS_MATERIAL marker_materials[1] = {
#include "collision/collision/marker_materials.inc"
};

static Sint16 marker_meshes0[60] = {
#include "collision/collision/marker_meshes0.inc"
};

static Sint16 marker_meshes1[320] = {
#include "collision/collision/marker_meshes1.inc"
};

static NJS_MESHSET marker_meshsets[2] = {
#include "collision/collision/marker_meshsets.inc"
};

static NJS_POINT3 marker_points[92] = {
#include "collision/collision/marker_points.inc"
};

static NJS_VECTOR marker_normals[92] = {
#include "collision/collision/marker_normals.inc"
};

static NJS_MODEL marker_model = {
#include "collision/collision/marker_model.inc"
};

NJS_OBJECT marker_object = {
#include "collision/collision/marker_object.inc"
};

/* a cylinder 20 across and 20 high, of twelve sides */
static NJS_MATERIAL marker_cylinder_materials[1] = {
#include "collision/collision/marker_cylinder_materials.inc"
};

static Sint16 marker_cylinder_meshes0[72] = {
#include "collision/collision/marker_cylinder_meshes0.inc"
};

static Sint16 marker_cylinder_meshes1[48] = {
#include "collision/collision/marker_cylinder_meshes1.inc"
};

static NJS_MESHSET marker_cylinder_meshsets[2] = {
#include "collision/collision/marker_cylinder_meshsets.inc"
};

static NJS_POINT3 marker_cylinder_points[26] = {
#include "collision/collision/marker_cylinder_points.inc"
};

static NJS_VECTOR marker_cylinder_normals[26] = {
#include "collision/collision/marker_cylinder_normals.inc"
};

static NJS_MODEL marker_cylinder_model = {
#include "collision/collision/marker_cylinder_model.inc"
};

NJS_OBJECT marker_cylinder_object = {
#include "collision/collision/marker_cylinder_object.inc"
};

/* a cube 10 across, a color to each face */
static NJS_MATERIAL marker_cube_materials[6] = {
#include "collision/collision/marker_cube_materials.inc"
};

static Sint16 marker_cube_meshes0[4] = {
#include "collision/collision/marker_cube_meshes0.inc"
};

static Sint16 marker_cube_meshes1[4] = {
#include "collision/collision/marker_cube_meshes1.inc"
};

static Sint16 marker_cube_meshes2[4] = {
#include "collision/collision/marker_cube_meshes2.inc"
};

static Sint16 marker_cube_meshes3[4] = {
#include "collision/collision/marker_cube_meshes3.inc"
};

static Sint16 marker_cube_meshes4[4] = {
#include "collision/collision/marker_cube_meshes4.inc"
};

static Sint16 marker_cube_meshes5[4] = {
#include "collision/collision/marker_cube_meshes5.inc"
};

static NJS_MESHSET marker_cube_meshsets[6] = {
#include "collision/collision/marker_cube_meshsets.inc"
};

static NJS_POINT3 marker_cube_points[8] = {
#include "collision/collision/marker_cube_points.inc"
};

static NJS_VECTOR marker_cube_normals[8] = {
#include "collision/collision/marker_cube_normals.inc"
};

static NJS_MODEL marker_cube_model = {
#include "collision/collision/marker_cube_model.inc"
};

NJS_OBJECT marker_cube_object[1] = {
#include "collision/collision/marker_cube_object.inc"
};

/* a rougher sphere 20 across: two poles and five rings of six points */
static NJS_MATERIAL marker_ball_materials[1] = {
#include "collision/collision/marker_ball_materials.inc"
};

static Sint16 marker_ball_meshes0[36] = {
#include "collision/collision/marker_ball_meshes0.inc"
};

static Sint16 marker_ball_meshes1[96] = {
#include "collision/collision/marker_ball_meshes1.inc"
};

static NJS_MESHSET marker_ball_meshsets[2] = {
#include "collision/collision/marker_ball_meshsets.inc"
};

static NJS_POINT3 marker_ball_points[32] = {
#include "collision/collision/marker_ball_points.inc"
};

static NJS_VECTOR marker_ball_normals[32] = {
#include "collision/collision/marker_ball_normals.inc"
};

static NJS_MODEL marker_ball_model = {
#include "collision/collision/marker_ball_model.inc"
};

NJS_OBJECT marker_ball_object = {
#include "collision/collision/marker_ball_object.inc"
};

/* the upper half of a sphere 20 across, which draws a capsule's end */
static NJS_MATERIAL marker_dome_materials[1] = {
#include "collision/collision/marker_dome_materials.inc"
};

static Sint16 marker_dome_meshes0[24] = {
#include "collision/collision/marker_dome_meshes0.inc"
};

static Sint16 marker_dome_meshes1[96] = {
#include "collision/collision/marker_dome_meshes1.inc"
};

static NJS_MESHSET marker_dome_meshsets[2] = {
#include "collision/collision/marker_dome_meshsets.inc"
};

static NJS_POINT3 marker_dome_points[33] = {
#include "collision/collision/marker_dome_points.inc"
};

static NJS_VECTOR marker_dome_normals[33] = {
#include "collision/collision/marker_dome_normals.inc"
};

static NJS_MODEL marker_dome_model = {
#include "collision/collision/marker_dome_model.inc"
};

NJS_OBJECT marker_dome_object = {
#include "collision/collision/marker_dome_object.inc"
};

/* a tube 20 across and 20 high, of eight sides, which draws a capsule's
 * side */
static NJS_MATERIAL marker_tube_materials[1] = {
#include "collision/collision/marker_tube_materials.inc"
};

static Sint16 marker_tube_meshes0[32] = {
#include "collision/collision/marker_tube_meshes0.inc"
};

static NJS_MESHSET marker_tube_meshsets[1] = {
#include "collision/collision/marker_tube_meshsets.inc"
};

static NJS_POINT3 marker_tube_points[16] = {
#include "collision/collision/marker_tube_points.inc"
};

static NJS_VECTOR marker_tube_normals[16] = {
#include "collision/collision/marker_tube_normals.inc"
};

static NJS_MODEL marker_tube_model = {
#include "collision/collision/marker_tube_model.inc"
};

NJS_OBJECT marker_tube_object = {
#include "collision/collision/marker_tube_object.inc"
};

static Sint8 dat_8c1b010c = 0;
static Sint8 dat_8c1b010d = 0;

struct near_task near_tasks[NEAR_TASKS + 1];
struct near_task near_rings[NEAR_RINGS + 1];
struct near_task near_tasks_player1[NEAR_TASKS + 1];
struct near_task near_rings_player1[NEAR_RINGS + 1];
Uint16 collision_player_count;
Uint16 collision_attack_count;
Uint16 collision_object_count;
Uint16 collision_mutual_object_count;
Uint16 collision_pickup_count;
Uint16 collision_ring_count;
Uint16 collision_wall_count;
struct collision_work *collision_players[8];
struct collision_work *collision_attacks[128];
struct collision_work *collision_items[128];
struct collision_work *collision_enemies[256];
static struct collision_work *collision_objects[512];
static struct collision_work *collision_mutual_objects[128];
static struct collision_work *collision_pickups[256];
static struct collision_work *collision_rings[256];
static struct collision_work *collision_walls[256];
/* 512 bytes of unknown type, which no code names */
static Uint8 dat_8c750e40[0x200];
/* how many tasks each near list holds */
static Uint16 near_task_count;
static Uint16 near_ring_count;
static Uint16 near_task_count_player1;
static Uint16 near_ring_count_player1;

/* set the constant material to the color of the shape's attack, from green
 * at 0 to red at 3 */
static void shape_set_material(struct collision_shape *shape)
{
    switch (shape->levels & SHAPE_ATTACK) {
    case 0:
        draw_set_material(0.03F, 0.0F, 1.0F, 0.0F);
        break;
    case 1:
        draw_set_material(0.03F, 0.5F, 0.5F, 0.0F);
        break;
    case 2:
        draw_set_material(0.03F, 0.5F, 0.25F, 0.0F);
        break;
    case 3:
        draw_set_material(0.03F, 1.0F, 0.0F, 0.0F);
        break;
    }
}

/* each of these draws its kind of shape as a marker at pos, scaled to the
 * shape's sizes and turned as the shape is. shape_draw_bound alone ignores
 * pos, and draws about its task's position */

static void shape_draw_sphere(struct collision_shape *shape, NJS_POINT3 *pos)
{
    Float scale = shape->radius * MARKER_FIT;

    njPushMatrix(NULL);
    njTranslate(NULL, pos->x, pos->y, pos->z);
    njScale(NULL, scale, scale, scale);
    shape_set_material(shape);
    njSimpleDrawObject(&marker_object);
    njPopMatrix(1);
    /* @hack redundant: a return where the function ends, which has SHC make a
     * frame with ADD #-4,R15 and store shape in it with MOV.L R4,@R15
     * (0x8c02f326, 0x8c02f32e); without it, MOV.L R4,@-R15 does both, the
     * function runs 2 bytes short and all 42 words differ */
    return; /* NOLINT(readability-redundant-control-flow) */
}

static void shape_draw_cylinder(struct collision_shape *shape, NJS_POINT3 *pos)
{
    Float radius = shape->radius * MARKER_FIT;
    Float height = shape->size_y * MARKER_FIT;

    njPushMatrix(NULL);
    njTranslate(NULL, pos->x, pos->y, pos->z);
    njScale(NULL, radius, height, radius);
    shape_set_material(shape);
    njSimpleDrawObject(&marker_cylinder_object);
    njPopMatrix(1);
}

static void shape_draw_cylinder2(struct task_work *work,
                                 struct collision_shape *shape, NJS_POINT3 *pos)
{
    Float radius = shape->radius * MARKER_FIT;
    Float height = shape->size_y * MARKER_FIT;

    njPushMatrix(NULL);
    njTranslate(NULL, pos->x, pos->y, pos->z);
    if (shape->flags & SHAPE_TURN_YZX) {
        if (!(shape->flags & SHAPE_IN_WORLD) &&
            !(shape->flags & SHAPE_UNTURNED)) {
            TURN(njRotateY, work->ang.y);
            TURN(njRotateZ, work->ang.z);
            TURN(njRotateX, work->ang.x);
        }
        TURN(njRotateY, shape->ang.y);
        TURN(njRotateZ, shape->ang.z);
        TURN(njRotateX, shape->ang.x);
    } else {
        if (!(shape->flags & SHAPE_IN_WORLD) &&
            !(shape->flags & SHAPE_UNTURNED)) {
            TURN(njRotateZ, work->ang.z);
            TURN(njRotateX, work->ang.x);
            TURN(njRotateY, work->ang.y);
        }
        TURN(njRotateZ, shape->ang.z);
        TURN(njRotateX, shape->ang.x);
        TURN(njRotateY, shape->ang.y);
    }
    shape_set_material(shape);
    njScale(NULL, radius, height, radius);
    shape_set_material(shape);
    njSimpleDrawObject(&marker_cylinder_object);
    njPopMatrix(1);
}

static void shape_draw_rectangle(struct task_work *work,
                                 struct collision_shape *shape, NJS_POINT3 *pos)
{
    Float x = shape->radius * MARKER_FIT;
    Float y = shape->size_y * MARKER_FIT;
    Float z = shape->size_z * MARKER_FIT;

    njPushMatrix(NULL);
    njTranslate(NULL, pos->x, pos->y, pos->z);
    if ((shape->flags & SHAPE_IN_WORLD) || (shape->flags & SHAPE_UNTURNED)) {
        TURN(njRotateY, shape->ang.y);
    } else {
        TURN(njRotateY, work->ang.y + shape->ang.y);
    }
    njScale(NULL, x * 2.0F, y * 2.0F, z * 2.0F);
    shape_set_material(shape);
    njSimpleDrawObject(marker_cube_object);
    njPopMatrix(1);
}

static void shape_draw_rectangle2(struct task_work *work,
                                  struct collision_shape *shape,
                                  NJS_POINT3 *pos)
{
    Float x = shape->radius * MARKER_FIT;
    Float y = shape->size_y * MARKER_FIT;
    Float z = shape->size_z * MARKER_FIT;

    njPushMatrix(NULL);
    njTranslate(NULL, pos->x, pos->y, pos->z);
    if ((shape->flags & SHAPE_IN_WORLD) || (shape->flags & SHAPE_UNTURNED)) {
        TURN(njRotateY, shape->ang.y);
    } else {
        TURN(njRotateY, work->ang.y + shape->ang.y);
    }
    TURN(njRotateZ, shape->ang.z);
    njScale(NULL, x * 2.0F, y * 2.0F, z * 2.0F);
    shape_set_material(shape);
    njSimpleDrawObject(marker_cube_object);
    njPopMatrix(1);
}

static void shape_draw_capsule(struct task_work *work,
                               struct collision_shape *shape, NJS_POINT3 *pos)
{
    Float radius = shape->radius * MARKER_FIT;
    Float height = shape->size_y * MARKER_FIT;

    njPushMatrix(NULL);
    njTranslate(NULL, pos->x, pos->y, pos->z);
    if (shape->flags & SHAPE_TURN_YZX) {
        if (!(shape->flags & SHAPE_IN_WORLD) &&
            !(shape->flags & SHAPE_UNTURNED)) {
            TURN(njRotateY, work->ang.y);
            TURN(njRotateZ, work->ang.z);
            TURN(njRotateX, work->ang.x);
        }
        TURN(njRotateY, shape->ang.y);
        TURN(njRotateZ, shape->ang.z);
        TURN(njRotateX, shape->ang.x);
    } else {
        if (!(shape->flags & SHAPE_IN_WORLD) &&
            !(shape->flags & SHAPE_UNTURNED)) {
            TURN(njRotateZ, work->ang.z);
            TURN(njRotateX, work->ang.x);
            TURN(njRotateY, work->ang.y);
        }
        TURN(njRotateZ, shape->ang.z);
        TURN(njRotateX, shape->ang.x);
        TURN(njRotateY, shape->ang.y);
    }
    shape_set_material(shape);

    njPushMatrix(NULL);
    njScale(NULL, radius, height, radius);
    njSimpleDrawObject(&marker_tube_object);
    njPopMatrix(1);

    njPushMatrix(NULL);
    njTranslate(NULL, 0.0F, shape->size_y, 0.0F);
    njScale(NULL, radius, radius, radius);
    njSimpleDrawObject(&marker_dome_object);
    njPopMatrix(1);

    njPushMatrix(NULL);
    njTranslate(NULL, 0.0F, -shape->size_y, 0.0F);
    njScale(NULL, radius, radius, radius);
    njRotateX(NULL, NJM_DEG_ANG(180));
    njSimpleDrawObject(&marker_dome_object);
    njPopMatrix(1);

    njPopMatrix(1);
}

/* a flat box with an arrow out of its face */
static void shape_draw_wall(struct task_work *work,
                            struct collision_shape *shape, NJS_POINT3 *pos)
{
    Float x = shape->radius * MARKER_FIT;
    Float y = shape->size_y * MARKER_FIT;
    /* @hack redundant: z, set and never read, which keeps x at R15+12 and y
     * at R15+8 (0x8c02fa2c, 0x8c02fa36), and njSimpleDrawObject and
     * marker_cube_object from trading R8 and R9; without it, 12 of 180 words
     * differ */
    Float z = shape->size_z * MARKER_FIT;

    njPushMatrix(NULL);
    njTranslate(NULL, pos->x, pos->y, pos->z);
    if ((shape->flags & SHAPE_IN_WORLD) || (shape->flags & SHAPE_UNTURNED)) {
        TURN(njRotateY, shape->ang.y);
    } else {
        TURN(njRotateY, work->ang.y + shape->ang.y);
    }

    njPushMatrix(NULL);
    njTranslate(NULL, 0.0F, 0.0F, 10.0F);
    njScale(NULL, 0.1F, 0.1F, 2.0F);
    shape_set_material(shape);
    njSimpleDrawObject(marker_cube_object);
    njPopMatrix(1);

    njPushMatrix(NULL);
    njTranslate(NULL, 0.0F, 0.0F, 20.0F);
    njRotateX(NULL, NJM_DEG_ANG(45));
    njTranslate(NULL, 0.0F, 0.0F, -3.0F);
    njScale(NULL, 0.1F, 0.1F, 0.7F);
    shape_set_material(shape);
    njSimpleDrawObject(marker_cube_object);
    njPopMatrix(1);

    njPushMatrix(NULL);
    njTranslate(NULL, 0.0F, 0.0F, 20.0F);
    njRotateX(NULL, NJM_DEG_ANG(-45));
    njTranslate(NULL, 0.0F, 0.0F, -3.0F);
    njScale(NULL, 0.1F, 0.1F, 0.7F);
    shape_set_material(shape);
    njSimpleDrawObject(marker_cube_object);
    njPopMatrix(1);

    njScale(NULL, x * 2.0F, y * 2.0F, 0.1F);
    shape_set_material(shape);
    njSimpleDrawObject(marker_cube_object);
    njPopMatrix(1);
}

/* draw the bound's circle, of its radius about its task's position, in
 * BOUND_SEGMENTS lines */
static void shape_draw_bound(struct task_work *work,
                             struct collision_shape *shape, NJS_POINT3 *pos)
{
    NJS_POINT3 edge;
    NJS_POINT3 from;
    NJS_POINT3 to;
    Angle ang;
    Angle next;
    int i;
    Float radius;
    /* @hack redundant: height, set and never read, which keeps njCalcPoint in
     * R8, njTranslateV in R9 and the two copies of &_unit_matrix_ at R15+8
     * and R15+12 (0x8c02fc1c, 0x8c02fc1e); without it, 26 of 253 words
     * differ */
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    Float height;

    radius = shape->radius;
    height = shape->size_y;
    edge.x = radius;
    edge.y = 0.0F;
    edge.z = 0.0F;
    next = BOUND_TURN;
    /* @hack spelling: the loop as a while, with i set ahead of ang and stepped
     * ahead of next and ang, which keeps ang in R12 and next in R13
     * (0x8c02fc2c, 0x8c02fc54); as a for loop, the two trade and 65 of 253
     * words differ */
    i = 0;
    ang = 0;
    while (i < BOUND_SEGMENTS) {
        njPushMatrix(&_unit_matrix_);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, ang);
        njCalcPoint(NULL, &edge, &from);
        njPopMatrix(1);
        njPushMatrix(&_unit_matrix_);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, next);
        njCalcPoint(NULL, &edge, &to);
        njPopMatrix(1);
        draw_line_v(&from, &to);
        i++;
        next += BOUND_TURN;
        ang += BOUND_TURN;
    }
}

/* draw the shape once a step, however many checks it's in */
static void shape_draw(struct task_work *work, struct collision_shape *shape,
                       NJS_POINT3 *pos)
{
    if (shape->flags & SHAPE_DRAWN) {
        return;
    }
    switch (shape->flags & SHAPE_KIND) {
    case SHAPE_SPHERE:
        shape_draw_sphere(shape, pos);
        break;
    case SHAPE_CYLINDER:
        shape_draw_cylinder(shape, pos);
        break;
    case SHAPE_CYLINDER2:
        shape_draw_cylinder2(work, shape, pos);
        break;
    case SHAPE_CAPSULE:
        shape_draw_capsule(work, shape, pos);
        break;
    case SHAPE_RECTANGLE:
        shape_draw_rectangle(work, shape, pos);
        break;
    case SHAPE_RECTANGLE2:
        shape_draw_rectangle2(work, shape, pos);
        break;
    case SHAPE_WALL:
        shape_draw_wall(work, shape, pos);
        break;
    case SHAPE_BOUND:
        shape_draw_bound(work, shape, pos);
        break;
    }
    shape->flags |= SHAPE_DRAWN;
}

void collision_draw_radius(struct collision_work *collision)
{
    struct task_work *work = collision->owner->work;
    Float scale = collision->radius * MARKER_FIT;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njScale(NULL, scale, scale, scale);
    draw_set_material(-0.5F, 0.5F, 0.5F, 0.0F);
    njSimpleDrawObject(&marker_object);
    njPopMatrix(1);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}

void collision_set_radius(struct collision_work *collision)
{
    struct collision_shape *shape;
    struct collision_shape *end;
    Float reach = 0.0F;
    Float distance;
    Float radius;
    Float height;
    Float depth;

    end = collision->shapes + collision->count;
    for (shape = collision->shapes; shape < end; shape++) {
        radius = shape->radius;
        height = shape->size_y;
        depth = shape->size_z;
        if ((shape->flags & SHAPE_KIND) == SHAPE_CAPSULE) {
            distance = njScalor(&shape->center) + radius + height;
        } else {
            distance =
                njScalor(&shape->center) +
                njSqrt((radius * radius) + (height * height) + (depth * depth));
        }
        if (distance > reach) {
            reach = distance;
        }
    }
    collision->radius = reach * 1.01F;
    if (reach < 0.1F) {
        collision->radius = 25.0F;
    }
}

void collision_init(struct task *task, const struct collision_shape *shapes,
                    Uint8 kind, int unused, Uint8 count)
{
    struct collision_work *collision;
    struct collision_shape *copy;
    /* @hack spelling: count, the argument on the stack, copied to
     * shape_count, which leaves njPrintC R11 and the message R12 (0x8c03009a,
     * 0x8c03009c); without it, the two trade and 4 of 105 words differ */
    Uint8 shape_count = count;

    /* @bug memory: when the shapes' copy can't be allocated, the new record
     * leaks. BT/S at 0x8c03010a leaves for the exit with it unfreed, and
     * work->collision stays as it was */
    if ((collision = heap_calloc(1, sizeof(struct collision_work))) == NULL) {
        /* @hack spelling: njPrintC called through print, and the message held
         * in a local, which give njPrintC R11, the message R12, debug_hidden's
         * address R13 and the location R14 (0x8c030096 to 0x8c03009c);
         * called by name with the text, 78 of 105 words differ */
        void (*print)(Int loc, const char *text) = njPrintC;
        const char *message = "CANT GET COLLI_WORK!!!!";

        for (;;) {
            /* @hack spelling: the wait as a do loop, which reads debug_hidden
             * into R3 (MOV.B @R13,R3 at 0x8c03009e); as a while loop, it reads
             * into R2 and 2 of 105 words differ */
            do {
            } while (debug_hidden < 0);
            print(NJM_LOCATION(30, 40), message);
        }
    } else if ((copy = heap_calloc(shape_count,
                                   sizeof(struct collision_shape))) != NULL) {
        njMemCopy(copy, shapes, shape_count * sizeof(struct collision_shape));
        collision->shapes = copy;
        collision->flags |= COLLISION_OWN_SHAPES;
        collision->kind = kind;
        if (kind == COLLISION_ITEM || kind == COLLISION_ENEMY ||
            kind == COLLISION_RING) {
            collision->flags |= COLLISION_TARGET;
        }
        collision->count = shape_count;
        collision->owner = task;
        collision->flags &= ~COLLISION_HIT;
        task->work->collision = collision;
        if (kind == COLLISION_PLAYER || kind == COLLISION_ATTACK) {
            collision->radius = 25.0F;
        } else {
            collision_set_radius(collision);
        }
    }
}

/* clear what the last step's checks set: the record's hits and flags, its
 * shapes' hits, SHAPE_DRAWN and SHAPE_HURT, and its task's collision
 * flags. it also clears the record's flag 0x8000, which no C source sets */
static void collision_reset(struct task_work *work,
                            struct collision_work *collision)
{
    Uint16 count = collision->count;
    struct collision_shape *shape = collision->shapes;

    collision->flags &= ~0x8000;
    collision->own_shape = -1;
    collision->hit_shape = -1;
    collision->hit = NULL;
    while (count--) {
        shape->hit_shape = -1;
        shape->hit = NULL;
        shape->flags &= ~SHAPE_DRAWN;
        shape->levels &= ~SHAPE_HURT;
        shape++;
    }
    work->flags &= ~(COLLISION_DAMAGED | COLLISION_TOUCHED);
    collision->flags &=
        ~(COLLISION_HIT | COLLISION_HIT_TOP | COLLISION_HIT_SIDE |
          COLLISION_HIT_BOTTOM | COLLISION_HIT_END | COLLISION_PUSHED |
          COLLISION_HIT_KEPT);
}

/* the records a list has room for */
#define ROOM(list) ((int)(sizeof(list) / sizeof((list)[0])))

void collision_register(struct task_work *work)
{
    struct collision_work *collision = work->collision;

    if (edit_mode) {
        return;
    }
    if (collision == NULL) {
        return;
    }
    collision_reset(work, collision);
    switch (collision->kind) {
    case COLLISION_PLAYER:
        if (collision_player_count < ROOM(collision_players)) {
            collision_players[collision_player_count] = collision;
            collision_player_count++;
        }
        break;
    case COLLISION_ATTACK:
        if (collision_attack_count < ROOM(collision_attacks)) {
            collision_attacks[collision_attack_count] = collision;
            collision_attack_count++;
        }
        break;
    case COLLISION_ITEM:
        if (collision_item_count < ROOM(collision_items)) {
            collision_items[collision_item_count] = collision;
            collision_item_count++;
        }
        break;
    case COLLISION_ENEMY:
        if (collision_enemy_count < ROOM(collision_enemies)) {
            collision_enemies[collision_enemy_count] = collision;
            collision_enemy_count++;
        }
        break;
    case COLLISION_OBJECT:
        if (collision_object_count < ROOM(collision_objects)) {
            collision_objects[collision_object_count] = collision;
            collision_object_count++;
        }
        break;
    case COLLISION_OBJECT_MUTUAL:
        if (collision_mutual_object_count < ROOM(collision_mutual_objects)) {
            collision_mutual_objects[collision_mutual_object_count] = collision;
            collision_mutual_object_count++;
        }
        break;
    case COLLISION_PICKUP:
        if (collision_pickup_count < ROOM(collision_pickups)) {
            collision_pickups[collision_pickup_count] = collision;
            collision_pickup_count++;
        }
        break;
    case COLLISION_RING:
        if (collision_ring_count < ROOM(collision_rings)) {
            collision_rings[collision_ring_count] = collision;
            collision_ring_count++;
        }
        break;
    case COLLISION_WALL:
        if (collision_wall_count < ROOM(collision_walls)) {
            collision_walls[collision_wall_count] = collision;
            collision_wall_count++;
        }
        break;
    }
    /* @hack redundant: a return where the function ends, which lets each case
     * that enters the record, and an unmatched kind, return in place, as at
     * 0x8c0302a0 and 0x8c0302c4; without it, each branches to one shared
     * exit, 174 of 195 words differ and collision.c's code runs 96 bytes
     * short */
    return; /* NOLINT(readability-redundant-control-flow) */
}

void collision_clear_lists(void)
{
    collision_player_count = 0;
    collision_attack_count = 0;
    collision_item_count = 0;
    collision_enemy_count = 0;
    collision_object_count = 0;
    collision_mutual_object_count = 0;
    collision_pickup_count = 0;
    collision_ring_count = 0;
    collision_wall_count = 0;
    dat_8c1b010c = 0;
    dat_8c1b010d = 0;
}

/* set hit to other, but keep a player's or an attack's record in hit
 * against another kind's */
/* @hack spelling: the test in a loop of one pass, out of which SHC hoists
 * other != NULL into R8 (0x8c030514 to 0x8c03051e); as an if, 173 of
 * collision_hit's 176 words differ */
#define SET_HIT(hit, other)                                                    \
    do {                                                                       \
        if ((hit) != NULL) {                                                   \
            if (((hit)->kind == COLLISION_PLAYER ||                            \
                 (hit)->kind == COLLISION_ATTACK) &&                           \
                (other) != NULL && (other)->kind != COLLISION_PLAYER &&        \
                (other)->kind != COLLISION_ATTACK) {                           \
                break;                                                         \
            }                                                                  \
        }                                                                      \
        (hit) = (other);                                                       \
    } while (0)

/* note that the two records' current shapes touched, in both records and
 * both shapes, and mark both tasks COLLISION_TOUCHING */
static void collision_hit(struct task_work *work,
                          struct collision_work *collision,
                          struct task_work *other_work,
                          struct collision_work *other)
{
    Uint16 index = collision->current;
    Uint16 other_index = other->current;
    struct collision_shape *shape = &collision->shapes[index];
    struct collision_shape *other_shape = &other->shapes[other_index];

    if (!(collision->flags & COLLISION_HIT_KEPT)) {
        SET_HIT(collision->hit, other);
        collision->hit_shape = other_index;
        collision->own_shape = index;
        SET_HIT(shape->hit, other);
        shape->hit_shape = other_index;
        if (other_shape->flags & SHAPE_KEEPS_HIT) {
            collision->flags |= COLLISION_HIT_KEPT;
        }
    }
    collision->flags |= COLLISION_HIT;
    if (!(other->flags & COLLISION_HIT_KEPT)) {
        SET_HIT(other->hit, collision);
        other->hit_shape = index;
        other->own_shape = other_index;
        SET_HIT(other_shape->hit, collision);
        other_shape->hit_shape = index;
        if (shape->flags & SHAPE_KEEPS_HIT) {
            other->flags |= COLLISION_HIT_KEPT;
        }
    }
    other->flags |= COLLISION_HIT;
    work->flags |= COLLISION_TOUCHING;
    other_work->flags |= COLLISION_TOUCHING;
}

/* let each current shape hurt the other, by the rule beside SHAPE_ATTACK.
 * a hurt shape takes SHAPE_HURT, and its task COLLISION_DAMAGED */
static void collision_hurt(struct collision_work *collision,
                           struct collision_work *other)
{
    Uint16 index = collision->current;
    Uint16 other_index = other->current;
    struct task_work *work = collision->owner->work;
    struct task_work *other_work = other->owner->work;
    struct collision_shape *shape = &collision->shapes[index];
    struct collision_shape *other_shape = &other->shapes[other_index];
    Uint8 guard;
    Uint8 attack;
    Uint8 other_guard;
    Uint8 other_attack;

    shape->levels &= ~SHAPE_HURT;
    other_shape->levels &= ~SHAPE_HURT;
    attack = shape->levels & SHAPE_ATTACK;
    other_attack = other_shape->levels & SHAPE_ATTACK;
    guard = (shape->levels & SHAPE_GUARD) >> 2;
    other_guard = (other_shape->levels & SHAPE_GUARD) >> 2;
    if (other_shape->flags & SHAPE_HURTABLE) {
        if (attack > other_guard &&
            ((other->kind == COLLISION_PLAYER &&
              (shape->levels & SHAPE_HURTS_PLAYERS)) ||
             ((other->kind == COLLISION_ITEM ||
               other->kind == COLLISION_ENEMY) &&
              (shape->levels & SHAPE_HURTS_ENEMIES)) ||
             ((other->kind == COLLISION_OBJECT ||
               other->kind == COLLISION_OBJECT_MUTUAL) &&
              (shape->levels & SHAPE_HURTS_OBJECTS)))) {
            other_shape->levels |= SHAPE_HURT;
            other_work->flags |= COLLISION_DAMAGED;
        }
    }
    if (shape->flags & SHAPE_HURTABLE) {
        if (other_attack > guard &&
            ((collision->kind == COLLISION_PLAYER &&
              (other_shape->levels & SHAPE_HURTS_PLAYERS)) ||
             ((collision->kind == COLLISION_ITEM ||
               collision->kind == COLLISION_ENEMY) &&
              (other_shape->levels & SHAPE_HURTS_ENEMIES)) ||
             ((collision->kind == COLLISION_OBJECT ||
               collision->kind == COLLISION_OBJECT_MUTUAL) &&
              (other_shape->levels & SHAPE_HURTS_OBJECTS)))) {
            shape->levels |= SHAPE_HURT;
            work->flags |= COLLISION_DAMAGED;
        }
    }
}

/* 1 when pusher_shape pushes shape, and 0 otherwise. pusher_shape is a
 * shape of pusher, and shape one of pushed. it pushes when its push names
 * pushed's group, and shape's push holds no PUSH_FIRM of pusher's group. a
 * push marks pushed COLLISION_PUSHED. two players never push each other.
 * the result is a Sint8, which the callers sign-extend to test it */
static Sint8 try_mark_pushed(struct collision_work *pushed,
                             struct collision_shape *shape,
                             struct collision_work *pusher,
                             struct collision_shape *pusher_shape)
{
    int kind = pushed->kind;
    int pusher_kind = pusher->kind;
    int push = shape->push;
    int pusher_push = pusher_shape->push;

    if (kind == COLLISION_PLAYER && pusher_kind == COLLISION_PLAYER) {
        return 0;
    }
    if ((kind == COLLISION_PLAYER && (pusher_push & PUSH_PLAYERS)) ||
        ((kind == COLLISION_ITEM || kind == COLLISION_ENEMY) &&
         (pusher_push & PUSH_ENEMIES)) ||
        ((kind == COLLISION_OBJECT || kind == COLLISION_OBJECT_MUTUAL) &&
         (pusher_push & PUSH_OBJECTS)) ||
        ((kind == COLLISION_PICKUP || kind == COLLISION_RING) &&
         (pusher_push & PUSH_OTHERS))) {
        if ((pusher_kind == COLLISION_PLAYER &&
             (push & PUSH_FIRM(PUSH_PLAYERS))) ||
            ((pusher_kind == COLLISION_ITEM ||
              pusher_kind == COLLISION_ENEMY) &&
             (push & PUSH_FIRM(PUSH_ENEMIES))) ||
            ((pusher_kind == COLLISION_OBJECT ||
              pusher_kind == COLLISION_OBJECT_MUTUAL) &&
             (push & PUSH_FIRM(PUSH_OBJECTS))) ||
            ((pusher_kind == COLLISION_PICKUP ||
              pusher_kind == COLLISION_RING) &&
             (push & PUSH_FIRM(PUSH_OTHERS)))) {
            return 0;
        }
        pushed->flags |= COLLISION_PUSHED;
        return 1;
    }
    return 0;
}

int collision_get_hit_player(struct task_work *work)
{
    struct collision_work *collision = work->collision;

    if (collision != NULL && collision->hit != NULL) {
        struct task_work *hit = collision->hit->owner->work;

        if (hit == player_work[0]) {
            return 0;
        } else if (hit == player_work[1]) {
            return 1;
        }
    }
    return -1;
}

/* the shape's center taken as it is, or turned with the task, or offset
 * from the task's position unturned */
void collision_get_shape_center(struct task_work *work,
                                struct collision_shape *shape, NJS_POINT3 *pos)
{
    if (shape->flags & SHAPE_IN_WORLD) {
        VECTOR_COPY(pos, &shape->center);
    } else if (!(shape->flags & SHAPE_UNTURNED)) {
        njPushMatrix(&_unit_matrix_);
        njTranslateV(NULL, &work->pos);
        if (shape->flags & SHAPE_TURN_YZX) {
            TURN(njRotateY, work->ang.y);
            TURN(njRotateZ, work->ang.z);
            TURN(njRotateX, work->ang.x);
        } else {
            TURN(njRotateZ, work->ang.z);
            TURN(njRotateX, work->ang.x);
            TURN(njRotateY, work->ang.y);
        }
        njCalcPoint(NULL, &shape->center, pos);
        njPopMatrix(1);
    } else {
        /* @hack spelling: one Float cursor stepped across work->pos's
         * members, then shape->center's, then pos's down from past its z, for
         * the image's @Rn+ loads and @-Rn stores; as the members, in the same
         * order, 25 of 127 words differ */
        Float *part = &work->pos.x;
        Float x = *part++;
        Float y = *part++;
        Float z = *part;
        Float cx;
        Float cy;
        Float cz;

        part = &shape->center.x;
        cx = *part++;
        cy = *part++;
        cz = *part;
        x += cx;
        y += cy;
        z += cz;
        part = &pos->z + 1;
        *--part = z;
        *--part = y;
        *--part = x;
    }
}

/* a copy of collision_get_shape_center, which the cylinder and capsule
 * checks and collision_get_hit_normal call */
static void shape_get_center(struct task_work *work,
                             struct collision_shape *shape, NJS_POINT3 *pos)
{
    if (shape->flags & SHAPE_IN_WORLD) {
        VECTOR_COPY(pos, &shape->center);
    } else if (!(shape->flags & SHAPE_UNTURNED)) {
        njPushMatrix(&_unit_matrix_);
        njTranslateV(NULL, &work->pos);
        if (shape->flags & SHAPE_TURN_YZX) {
            TURN(njRotateY, work->ang.y);
            TURN(njRotateZ, work->ang.z);
            TURN(njRotateX, work->ang.x);
        } else {
            TURN(njRotateZ, work->ang.z);
            TURN(njRotateX, work->ang.x);
            TURN(njRotateY, work->ang.y);
        }
        njCalcPoint(NULL, &shape->center, pos);
        njPopMatrix(1);
    } else {
        /* @hack spelling: one Float cursor stepped across work->pos's
         * members, then shape->center's, then pos's down from past its z, for
         * the image's @Rn+ loads and @-Rn stores; as the members, in the same
         * order, 25 of 127 words differ */
        Float *part = &work->pos.x;
        Float x = *part++;
        Float y = *part++;
        Float z = *part;
        Float cx;
        Float cy;
        Float cz;

        part = &shape->center.x;
        cx = *part++;
        cy = *part++;
        cz = *part;
        x += cx;
        y += cy;
        z += cz;
        part = &pos->z + 1;
        *--part = z;
        *--part = y;
        *--part = x;
    }
}

/* the center of a rectangle, a wall or a bound, as
 * collision_get_shape_center finds a shape's. it offsets an unturned
 * shape's center through the matrix too, not by sums */
static void shape_get_box_center(struct task_work *work,
                                 struct collision_shape *shape, NJS_POINT3 *pos)
{
    if (shape->flags & SHAPE_IN_WORLD) {
        VECTOR_COPY(pos, &shape->center);
    } else {
        njPushMatrix(&_unit_matrix_);
        njTranslateV(NULL, &work->pos);
        if (!(shape->flags & SHAPE_UNTURNED)) {
            if (shape->flags & SHAPE_TURN_YZX) {
                TURN(njRotateY, work->ang.y);
                TURN(njRotateZ, work->ang.z);
                TURN(njRotateX, work->ang.x);
            } else {
                TURN(njRotateZ, work->ang.z);
                TURN(njRotateX, work->ang.x);
                TURN(njRotateY, work->ang.y);
            }
        }
        njCalcPoint(NULL, &shape->center, pos);
        njPopMatrix(1);
    }
}

/* the middle of a shape that stands at its task's position */
static void shape_get_standing_center(struct task_work *work,
                                      struct collision_shape *shape,
                                      NJS_POINT3 *pos)
{
    pos->x = work->pos.x;
    pos->y = work->pos.y + shape->size_y;
    pos->z = work->pos.z;
}

/* each check tests collision's current shape against other's. on a touch
 * it notes the hit, and may push one or both apart. a check's Float result
 * reaches only the result of collide_pair or
 * collide_target, which no caller reads */

/* two spheres: on a touch, push both apart along the line between them */
static Float collide_sphere_sphere(struct collision_work *collision,
                                   struct collision_work *other)
{
    Uint16 index = collision->current;
    Uint16 other_index = other->current;
    struct collision_shape *shape = &collision->shapes[index];
    struct collision_shape *other_shape = &other->shapes[other_index];
    Float radius = shape->radius;
    Float other_radius = other_shape->radius;
    struct task_work *work = collision->owner->work;
    struct task_work *other_work = other->owner->work;
    NJS_POINT3 pos;
    NJS_POINT3 other_pos;
    NJS_VECTOR push;
    Float distance;
    Float overlap;

    collision_get_shape_center(work, shape, &pos);
    collision_get_shape_center(other_work, other_shape, &other_pos);
    distance = ((pos.x - other_pos.x) * (pos.x - other_pos.x)) +
               ((pos.y - other_pos.y) * (pos.y - other_pos.y)) +
               ((pos.z - other_pos.z) * (pos.z - other_pos.z));
    if ((shape->radius + other_shape->radius) *
            (shape->radius + other_shape->radius) >
        distance) {
        collision_hit(work, collision, other_work, other);
        collision->flags |= COLLISION_HIT;
        other->flags |= COLLISION_HIT;
        collision_hurt(collision, other);
        push.x = pos.x - other_pos.x;
        if (collision->kind == COLLISION_PLAYER &&
            other->kind == COLLISION_PLAYER) {
            push.y = 0.0F;
        } else {
            push.y = pos.y - other_pos.y;
        }
        push.z = pos.z - other_pos.z;
        overlap = radius + other_radius - njSqrt(distance);
        if (njScalor(&push) == 0.0F) {
            push.x = push.y = 0.0F;
            push.z = 0.1F;
        } else {
            njUnitVector(&push);
        }
        push.x *= overlap;
        push.y *= overlap;
        push.z *= overlap;
        if (try_mark_pushed(collision, shape, other, other_shape)) {
            work->pos.x += push.x;
            work->pos.y += push.y;
            work->pos.z += push.z;
        }
        /* NOLINTNEXTLINE(readability-suspicious-call-argument) */
        if (try_mark_pushed(other, other_shape, collision, shape)) {
            other_work->pos.x -= push.x;
            other_work->pos.y -= push.y;
            other_work->pos.z -= push.z;
        }
    }
    if (debug_hidden >= 0) {
        shape_draw(work, shape, &pos);
        if (debug_hidden >= 0) {
            shape_draw(other_work, other_shape, &other_pos);
        }
    }
    return distance;
}

/* a shape standing on its task's position against a cylinder: land on its
 * top, bump into its bottom, or be pushed out of its side */
static Float collide_standing_cylinder(struct collision_work *collision,
                                       struct collision_work *other)
{
    Uint16 index = collision->current;
    Uint16 other_index = other->current;
    struct collision_shape *shape = &collision->shapes[index];
    struct collision_shape *other_shape = &other->shapes[other_index];
    Float radius = shape->radius;
    Float other_radius = other_shape->radius;
    Float height = shape->size_y;
    Float other_height = other_shape->size_y;
    Float depth = shape->size_z;
    /* @hack redundant: reach, set and never read, which gives other_height
     * FR13 (0x8c030f46) and height FR12 (0x8c030f48). other_bottom then takes
     * FR13 from other_height (0x8c030fa6). without it, 15 of 403 words
     * differ */
    Float reach = collision->radius;
    struct task_work *work = collision->owner->work;
    struct task_work *other_work = other->owner->work;
    NJS_POINT3 pos;
    NJS_POINT3 other_pos;
    NJS_VECTOR push;
    Float distance;
    Float gap;
    Float top;
    Float bottom;
    Float other_top;
    Float other_bottom;

    shape_get_standing_center(work, shape, &pos);
    shape_get_center(other_work, other_shape, &other_pos);
    distance = ((pos.x - other_pos.x) * (pos.x - other_pos.x)) +
               ((pos.z - other_pos.z) * (pos.z - other_pos.z));
    top = pos.y + height;
    bottom = pos.y - height;
    other_top = other_pos.y + other_height;
    other_bottom = other_pos.y - other_height;
    if (other_top > bottom && bottom > other_top - 3.5F &&
        (other_radius + depth) * (other_radius + depth) > distance) {
        if (debug_hidden >= 0) {
            njPrintC(NJM_LOCATION(10, 15), "UP");
        }
        if (collision->hit == NULL) {
            collision_hit(work, collision, other_work, other);
            collision_hurt(collision, other);
            other_work->flags |= COLLISION_UNDER;
            other->flags |= COLLISION_HIT_TOP;
        }
        collision->flags |= COLLISION_HIT_BOTTOM;
        if (try_mark_pushed(collision, shape, other, other_shape)) {
            work->flags &= ~COLLISION_TOUCHING;
            work->flags |= COLLISION_ON_TOP;
            work->pos.y = other_top - 0.01F;
            work->ang.x = 0;
            work->ang.z = 0;
        }
        return distance;
    }
    if ((radius + other_radius) * (radius + other_radius) > distance) {
        if (top > other_bottom && other_bottom + 2.0F > top) {
            collision_hit(work, collision, other_work, other);
            collision_hurt(collision, other);
            other_work->flags |= COLLISION_ON_TOP;
            other->flags |= COLLISION_HIT_BOTTOM;
            collision->flags |= COLLISION_HIT_TOP;
            if (try_mark_pushed(collision, shape, other, other_shape)) {
                work->flags |= COLLISION_UNDER;
                work->flags &= ~COLLISION_TOUCHING;
                work->pos.y = other_bottom - (height * 2.0F) + 0.01F;
            }
            if (debug_hidden >= 0) {
                njPrintC(NJM_LOCATION(10, 15), "DOWN");
            }
        } else if (bottom + 1.5F < other_top - 3.5F && top > other_bottom) {
            if (debug_hidden >= 0) {
                njPrintC(NJM_LOCATION(10, 15), "SIDE");
            }
            collision_hit(work, collision, other_work, other);
            collision_hurt(collision, other);
            collision->flags |= COLLISION_HIT_SIDE;
            other->flags |= COLLISION_HIT_SIDE;
            if (try_mark_pushed(collision, shape, other, other_shape)) {
                push.x = pos.x - other_pos.x;
                push.y = 0.0F;
                push.z = pos.z - other_pos.z;
                gap = njScalor(&push) - shape->radius;
                gap -= other_shape->radius;
                if (gap < 0.0F) {
                    if (njScalor(&push) == 0.0F) {
                        push.x = push.y = 0.0F;
                        push.z = 0.1F;
                    } else {
                        njUnitVector(&push);
                    }
                    gap = -0.1F - gap;
                    push.x = push.x * gap * 0.7F;
                    push.z = push.z * gap * 0.7F;
                    work->pos.x += push.x;
                    work->pos.z += push.z;
                }
            }
        } else if (debug_hidden >= 0) {
            njPrintC(NJM_LOCATION(10, 15), "NONE");
        }
    }
    pos = shape->center;
    if (debug_hidden >= 0) {
        shape_draw(work, shape, &pos);
    }
    if (debug_hidden >= 0) {
        shape_draw(other_work, other_shape, &other_pos);
    }
    return distance;
}

/* a sphere against a cylinder. the two touch when they're close enough
 * across and the sphere's center lies within the cylinder's height grown by
 * the sphere's radius. neither is pushed */
static Float collide_sphere_cylinder(struct collision_work *collision,
                                     struct collision_work *other)
{
    Uint16 index = collision->current;
    Uint16 other_index = other->current;
    struct collision_shape *shape = &collision->shapes[index];
    struct collision_shape *other_shape = &other->shapes[other_index];
    Float radius = shape->radius;
    /* @hack redundant: height, set and never read, which keeps shape and
     * other_shape from trading R11 and R12 (0x8c0312ba on); without it, 15
     * of 125 words differ */
    Float height = shape->size_y;
    Float other_radius = other_shape->radius;
    Float other_height = other_shape->size_y;
    struct task_work *work = collision->owner->work;
    struct task_work *other_work = other->owner->work;
    NJS_POINT3 pos;
    NJS_POINT3 other_pos;
    Float distance;
    Float top;
    Float bottom;

    collision_get_shape_center(work, shape, &pos);
    shape_get_center(other_work, other_shape, &other_pos);
    distance = ((pos.x - other_pos.x) * (pos.x - other_pos.x)) +
               ((pos.z - other_pos.z) * (pos.z - other_pos.z));
    top = other_pos.y + other_height + radius;
    bottom = other_pos.y - radius - other_height;
    if ((radius + other_radius) * (radius + other_radius) > distance) {
        if (top > pos.y && pos.y > bottom) {
            collision_hit(work, collision, other_work, other);
            collision_hurt(collision, other);
        }
    }
    if (debug_hidden >= 0) {
        shape_draw(work, shape, &pos);
        if (debug_hidden >= 0) {
            shape_draw(other_work, other_shape, &other_pos);
        }
    }
    return distance;
}

/* two cylinders, which touch when they're close enough across and their
 * heights overlap. a touch pushes both apart across x and z */
static Float collide_cylinder_cylinder(struct collision_work *collision,
                                       struct collision_work *other)
{
    Uint16 index = collision->current;
    Uint16 other_index = other->current;
    struct collision_shape *shape = &collision->shapes[index];
    struct collision_shape *other_shape = &other->shapes[other_index];
    Float radius = shape->radius;
    Float other_radius = other_shape->radius;
    Float height = shape->size_y;
    Float other_height = other_shape->size_y;
    /* @hack redundant: depth and other_depth, set and never read, which give
     * height FR14 and other_height FR13 (0x8c031412, 0x8c031414); without
     * them, 26 of 244 words differ */
    Float depth = shape->size_z;
    Float other_depth = other_shape->size_z;
    struct task_work *work = collision->owner->work;
    struct task_work *other_work = other->owner->work;
    NJS_POINT3 pos;
    NJS_POINT3 other_pos;
    NJS_VECTOR push;
    Float distance;
    Float overlap;
    Float top;
    Float bottom;
    Float other_top;
    Float other_bottom;

    shape_get_center(work, shape, &pos);
    shape_get_center(other_work, other_shape, &other_pos);
    distance = ((pos.x - other_pos.x) * (pos.x - other_pos.x)) +
               ((pos.z - other_pos.z) * (pos.z - other_pos.z));
    top = pos.y + height;
    bottom = pos.y - height;
    other_top = other_pos.y + other_height;
    other_bottom = other_pos.y - other_height;
    if (distance < (radius + other_radius) * (radius + other_radius)) {
        if (bottom < other_top && top > other_bottom) {
            collision_hit(work, collision, other_work, other);
            collision_hurt(collision, other);
            push.x = pos.x - other_pos.x;
            push.y = 0.0F;
            push.z = pos.z - other_pos.z;
            overlap = radius + other_radius - njSqrt(distance) - 0.001F;
            if (njScalor(&push) == 0.0F) {
                push.x = push.y = 0.0F;
                push.z = 0.1F;
            } else {
                njUnitVector(&push);
            }
            push.x = push.x * overlap * 0.4F;
            push.y = 0.0F;
            push.z = push.z * overlap * 0.4F;
            collision->flags |= COLLISION_HIT_SIDE;
            other->flags |= COLLISION_HIT_SIDE;
            if (try_mark_pushed(collision, shape, other, other_shape)) {
                work->pos.x += push.x;
                work->pos.z += push.z;
            }
            /* NOLINTNEXTLINE(readability-suspicious-call-argument) */
            if (try_mark_pushed(other, other_shape, collision, shape)) {
                other_work->pos.x -= push.x;
                other_work->pos.z -= push.z;
            }
        }
    }
    if (debug_hidden >= 0) {
        shape_draw(work, shape, &pos);
        if (debug_hidden >= 0) {
            shape_draw(other_work, other_shape, &other_pos);
        }
    }
    return distance;
}

/* the way the shape's top faces, as the task and the shape turn it */
static void shape_get_up(struct task_work *work, struct collision_shape *shape,
                         NJS_VECTOR *up)
{
    NJS_VECTOR unit;

    unit.x = 0.0F;
    unit.y = 1.0F;
    unit.z = 0.0F;
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    if (shape->flags & SHAPE_TURN_YZX) {
        if (!(shape->flags & SHAPE_IN_WORLD) &&
            !(shape->flags & SHAPE_UNTURNED)) {
            TURN(njRotateY, work->ang.y);
            TURN(njRotateZ, work->ang.z);
            TURN(njRotateX, work->ang.x);
        }
        TURN(njRotateY, shape->ang.y);
        TURN(njRotateZ, shape->ang.z);
        TURN(njRotateX, shape->ang.x);
    } else {
        if (!(shape->flags & SHAPE_IN_WORLD) &&
            !(shape->flags & SHAPE_UNTURNED)) {
            TURN(njRotateZ, work->ang.z);
            TURN(njRotateX, work->ang.x);
            TURN(njRotateY, work->ang.y);
        }
        TURN(njRotateZ, shape->ang.z);
        TURN(njRotateX, shape->ang.x);
        TURN(njRotateY, shape->ang.y);
    }
    njCalcVector(NULL, &unit, up);
    njPopMatrix(1);
}

/* a sphere against a cylinder2. the axis runs from the cylinder's center
 * along its up, each way, by its size_y plus the sphere's. the two touch
 * when the sphere's center lies within the sum of their radii of the axis,
 * at a point between the ends. a touch more than 3 inside an end pushes
 * both apart through the side. a touch within 3 of an end measures the
 * sphere by its size_z in place of its radius. when it still reaches the
 * axis, the sphere alone moves along the axis by the distance left to the
 * end. an end that faces up counts as a top the sphere stands on */
static Float collide_sphere_cylinder2(struct collision_work *collision,
                                      struct collision_work *other)
{
    Uint16 index = collision->current;
    Uint16 other_index = other->current;
    struct collision_shape *shape = &collision->shapes[index];
    struct collision_shape *other_shape = &other->shapes[other_index];
    Float radius = shape->radius;
    /* @hack spelling: distance declared between the radii, which keeps
     * other_radius in FR12 (0x8c0317c6); declared after them, other_radius
     * goes to the frame and 390 of 585 words differ */
    Float distance;
    Float other_radius = other_shape->radius;
    Float height = shape->size_y;
    Float other_height = other_shape->size_y;
    Float depth = shape->size_z;
    struct task_work *work = collision->owner->work;
    struct task_work *other_work = other->owner->work;
    NJS_POINT3 pos;
    NJS_POINT3 other_pos;
    NJS_POINT3 ends[2];
    NJS_VECTOR up;
    NJS_LINE line;
    NJS_POINT3 nearest;
    NJS_VECTOR to_top;
    NJS_VECTOR to_bottom;
    NJS_VECTOR push;
    NJS_VECTOR axis;
    Float across;
    Float dx;
    Float dy;
    Float dz;
    Float overlap;

    collision_get_shape_center(work, shape, &pos);
    shape_get_up(other_work, other_shape, &up);
    shape_get_center(other_work, other_shape, &other_pos);
    /* each end lies other_height plus the sphere's own size_y from the
     * center */
    ends[0].x = other_pos.x + ((height + other_height) * up.x);
    ends[0].y = other_pos.y + ((height + other_height) * up.y);
    ends[0].z = other_pos.z + ((height + other_height) * up.z);
    ends[1].x = other_pos.x - ((height + other_height) * up.x);
    ends[1].y = other_pos.y - ((height + other_height) * up.y);
    ends[1].z = other_pos.z - ((height + other_height) * up.z);
    line.px = ends[1].x;
    line.py = ends[1].y;
    line.pz = ends[1].z;
    line.vx = up.x;
    line.vy = up.y;
    line.vz = up.z;
    distance = njDistanceP2L(&pos, &line, &nearest);
    to_top.x = ends[0].x - nearest.x;
    to_top.y = ends[0].y - nearest.y;
    to_top.z = ends[0].z - nearest.z;
    to_bottom.x = ends[1].x - nearest.x;
    to_bottom.y = ends[1].y - nearest.y;
    to_bottom.z = ends[1].z - nearest.z;
    if (radius + other_radius > distance) {
        if (njInnerProduct(&to_top, &to_bottom) < 0.0F) {
            dx = other_pos.x - nearest.x;
            dy = other_pos.y - nearest.y;
            dz = other_pos.z - nearest.z;
            across = (dx * dx) + (dy * dy) + (dz * dz);
            if (debug_hidden >= 0) {
                draw_line(pos.x, pos.y, pos.z, nearest.x, nearest.y, nearest.z);
            }
            collision_hit(work, collision, other_work, other);
            collision_hurt(collision, other);
            other->flags |= COLLISION_HIT_SIDE;
            if ((height + other_height - 3.0F) *
                    (height + other_height - 3.0F) >
                across) {
                push.x = pos.x - nearest.x;
                push.y = pos.y - nearest.y;
                push.z = pos.z - nearest.z;
                overlap = radius + other_radius - distance - 0.1F;
                if (njScalor(&push) == 0.0F) {
                    push.x = push.y = 0.0F;
                    push.z = 0.1F;
                } else {
                    njUnitVector(&push);
                }
                push.x *= overlap;
                push.y *= overlap;
                push.z *= overlap;
                if (try_mark_pushed(collision, shape, other, other_shape)) {
                    work->pos.x += push.x;
                    work->pos.y += push.y;
                    work->pos.z += push.z;
                }
                if (debug_hidden >= 0) {
                    njPrintC(NJM_LOCATION(10, 17), "HIT CYLINDER2 SIDE");
                }
                /* NOLINTNEXTLINE(readability-suspicious-call-argument) */
                if (try_mark_pushed(other, other_shape, collision, shape)) {
                    other_work->pos.x -= push.x;
                    other_work->pos.y -= push.y;
                    other_work->pos.z -= push.z;
                }
            } else if (other_radius + depth > distance) {
                /* the way up, which the end must face to be stood on */
                static const NJS_VECTOR straight_up = {0.0F, 1.0F, 0.0F};

                axis = straight_up;
                push = nearest;
                njSubVector(&push, &other_pos);
                if (njScalor(&push) == 0.0F) {
                    push.x = push.y = 0.0F;
                    push.z = 0.1F;
                } else {
                    njUnitVector(&push);
                }
                if (njInnerProduct(&axis, &push) > END_UP) {
                    work->flags |= COLLISION_ON_TOP;
                    other_work->flags |= COLLISION_UNDER;
                    collision->flags |= COLLISION_HIT_BOTTOM;
                    other->flags |= COLLISION_HIT_TOP;
                }
                collision->flags |= COLLISION_HIT_END;
                other->flags |= COLLISION_HIT_END;
                push.x *= height + other_height - njSqrt(across);
                push.y *= height + other_height - njSqrt(across);
                push.z *= height + other_height - njSqrt(across);
                if (debug_hidden >= 0) {
                    njPrintC(NJM_LOCATION(10, 17), "HIT CYLINDER2 FLAT");
                }
                if (try_mark_pushed(collision, shape, other, other_shape)) {
                    njAddVector(&work->pos, &push);
                    work->flags &= ~COLLISION_TOUCHING;
                }
            }
        }
    }
    if (debug_hidden >= 0) {
        shape_draw(work, shape, &pos);
    }
    if (debug_hidden >= 0) {
        shape_draw(other_work, other_shape, &other_pos);
    }
    return distance;
}

/* a sphere against a capsule. when the nearest point of the axis lies
 * between the ends, a touch pushes both apart through the side. otherwise
 * the sphere meets the sphere of the capsule's radius about the upper end,
 * or failing that about the lower one. a touch there pushes the sphere
 * alone out of it */
static Float collide_sphere_capsule(struct collision_work *collision,
                                    struct collision_work *other)
{
    Uint16 index = collision->current;
    Uint16 other_index = other->current;
    struct collision_shape *shape = &collision->shapes[index];
    struct collision_shape *other_shape = &other->shapes[other_index];
    Float radius = shape->radius;
    /* @hack spelling: distance declared between the radii, which keeps the
     * side's overlap summed as written, radius - distance + other_radius
     * (0x8c031f5c on); declared after them, SHC subtracts distance from the
     * radii's sum in FR15 and 45 of 708 words differ */
    Float distance;
    Float other_radius = other_shape->radius;
    /* @hack redundant: height, set and never read, whose reference keeps
     * shape in R8 (0x8c031d7a), 0.1F in FR12 (0x8c031ed2) and -0.1F in FR13
     * (0x8c031ed6); without it, 629 of 708 words differ and collision.c's
     * code runs 32 bytes short */
    Float height = shape->size_y;
    Float other_height = other_shape->size_y;
    struct task_work *work = collision->owner->work;
    struct task_work *other_work = other->owner->work;
    NJS_POINT3 pos;
    NJS_POINT3 other_pos;
    NJS_POINT3 ends[2];
    NJS_VECTOR up;
    NJS_LINE line;
    NJS_POINT3 nearest;
    NJS_VECTOR to_top;
    NJS_VECTOR to_bottom;
    NJS_VECTOR push;
    Float overlap;
    /* @hack spelling: each of these set in its test, left of the < (0x8c0320e8
     * on); set right of it, the differences rotate through FR7 to FR9, the
     * radii trade frame slots and 21 of 708 words differ */
    Float top_distance;
    Float bottom_distance;

    collision_get_shape_center(work, shape, &pos);
    shape_get_up(other_work, other_shape, &up);
    shape_get_center(other_work, other_shape, &other_pos);
    ends[0].x = other_pos.x + (other_height * up.x);
    ends[0].y = other_pos.y + (other_height * up.y);
    ends[0].z = other_pos.z + (other_height * up.z);
    ends[1].x = other_pos.x - (other_height * up.x);
    ends[1].y = other_pos.y - (other_height * up.y);
    ends[1].z = other_pos.z - (other_height * up.z);
    line.px = ends[1].x;
    line.py = ends[1].y;
    line.pz = ends[1].z;
    line.vx = up.x;
    line.vy = up.y;
    line.vz = up.z;
    distance = njDistanceP2L(&pos, &line, &nearest);
    to_top.x = ends[0].x - nearest.x;
    to_top.y = ends[0].y - nearest.y;
    to_top.z = ends[0].z - nearest.z;
    to_bottom.x = ends[1].x - nearest.x;
    to_bottom.y = ends[1].y - nearest.y;
    to_bottom.z = ends[1].z - nearest.z;
    if (radius + other_radius > distance) {
        if (njInnerProduct(&to_top, &to_bottom) < 0.0F) {
            if (debug_hidden >= 0) {
                draw_line(pos.x, pos.y, pos.z, nearest.x, nearest.y, nearest.z);
            }
            collision_hit(work, collision, other_work, other);
            collision_hurt(collision, other);
            if (debug_hidden >= 0) {
                njPrintC(NJM_LOCATION(10, 17), "HIT_CAPSULE_SIDE");
            }
            other->flags |= COLLISION_HIT_SIDE;
            push.x = pos.x - nearest.x;
            push.y = pos.y - nearest.y;
            push.z = pos.z - nearest.z;
            overlap = radius - distance + other_radius - 0.1F;
            if (njScalor(&push) == 0.0F) {
                push.x = push.y = 0.0F;
                push.z = 0.1F;
            } else {
                njUnitVector(&push);
            }
            push.x *= overlap;
            push.y *= overlap;
            push.z *= overlap;
            if (try_mark_pushed(collision, shape, other, other_shape)) {
                work->pos.x += push.x;
                work->pos.y += push.y;
                work->pos.z += push.z;
            }
            /* NOLINTNEXTLINE(readability-suspicious-call-argument) */
            if (try_mark_pushed(other, other_shape, collision, shape)) {
                other_work->pos.x -= push.x;
                other_work->pos.y -= push.y;
                other_work->pos.z -= push.z;
            }
        } else if ((top_distance =
                        ((pos.x - ends[0].x) * (pos.x - ends[0].x)) +
                        ((pos.y - ends[0].y) * (pos.y - ends[0].y)) +
                        ((pos.z - ends[0].z) * (pos.z - ends[0].z))) <
                   (radius + other_radius) * (radius + other_radius)) {
            collision_hit(work, collision, other_work, other);
            collision_hurt(collision, other);
            if (try_mark_pushed(collision, shape, other, other_shape)) {
                push.x = pos.x - ends[0].x;
                push.y = pos.y - ends[0].y;
                push.z = pos.z - ends[0].z;
                overlap = radius + other_radius - njSqrt(top_distance) - 0.1F;
                if (njScalor(&push) == 0.0F) {
                    push.x = push.y = 0.0F;
                    push.z = 0.1F;
                } else {
                    njUnitVector(&push);
                }
                if (debug_hidden >= 0) {
                    njPrintC(NJM_LOCATION(10, 17), "HIT_CAPSULE_UPPER");
                }
                other->flags |= COLLISION_HIT_TOP;
                push.x *= overlap;
                push.y *= overlap;
                push.z *= overlap;
                work->pos.x += push.x;
                work->pos.y += push.y;
                work->pos.z += push.z;
            }
        } else if ((bottom_distance =
                        ((pos.x - ends[1].x) * (pos.x - ends[1].x)) +
                        ((pos.y - ends[1].y) * (pos.y - ends[1].y)) +
                        ((pos.z - ends[1].z) * (pos.z - ends[1].z))) <
                   (radius + other_radius) * (radius + other_radius)) {
            collision_hit(work, collision, other_work, other);
            collision_hurt(collision, other);
            if (try_mark_pushed(collision, shape, other, other_shape)) {
                push.x = pos.x - ends[1].x;
                push.y = pos.y - ends[1].y;
                push.z = pos.z - ends[1].z;
                overlap =
                    radius + other_radius - njSqrt(bottom_distance) - 0.1F;
                if (njScalor(&push) == 0.0F) {
                    push.x = push.y = 0.0F;
                    push.z = 0.1F;
                } else {
                    njUnitVector(&push);
                }
                if (debug_hidden >= 0) {
                    njPrintC(NJM_LOCATION(10, 17), "HIT_CAPSULE_LOWER");
                }
                other->flags |= COLLISION_HIT_BOTTOM;
                push.x *= overlap;
                push.y *= overlap;
                push.z *= overlap;
                work->pos.x += push.x;
                work->pos.y += push.y;
                work->pos.z += push.z;
            }
        }
    }
    if (debug_hidden >= 0) {
        shape_draw(work, shape, &pos);
        if (debug_hidden >= 0) {
            shape_draw(other_work, other_shape, &other_pos);
        }
    }
    return distance;
}

/* a cylinder against a capsule. it finds both shapes' ends and draws the
 * shapes, but tests nothing, so the two never touch */
/* NOLINTBEGIN(clang-diagnostic-unused-but-set-variable) */
static Float collide_cylinder_capsule(struct collision_work *collision,
                                      struct collision_work *other)
{
    Uint16 index = collision->current;
    Uint16 other_index = other->current;
    struct collision_shape *shape = &collision->shapes[index];
    struct collision_shape *other_shape = &other->shapes[other_index];
    struct task_work *other_work = other->owner->work;
    struct task_work *work = collision->owner->work;
    Float other_height = other_shape->size_y;
    Float height = shape->size_y;
    NJS_POINT3 pos;
    NJS_POINT3 other_pos;
    NJS_POINT3 ends[2];
    NJS_POINT3 other_ends[2];
    NJS_VECTOR up;
    NJS_VECTOR other_up;

    shape_get_up(work, shape, &up);
    shape_get_center(work, shape, &pos);
    ends[0].x = pos.x + (height * up.x);
    ends[0].y = pos.y + (height * up.y);
    ends[0].z = pos.z + (height * up.z);
    ends[1].x = pos.x - (height * up.x);
    ends[1].y = pos.y - (height * up.y);
    ends[1].z = pos.z - (height * up.z);
    shape_get_up(other_work, other_shape, &other_up);
    shape_get_center(other_work, other_shape, &other_pos);
    other_ends[0].x = other_pos.x + (other_height * other_up.x);
    other_ends[0].y = other_pos.y + (other_height * other_up.y);
    other_ends[0].z = other_pos.z + (other_height * other_up.z);
    other_ends[1].x = other_pos.x - (other_height * other_up.x);
    other_ends[1].y = other_pos.y - (other_height * other_up.y);
    other_ends[1].z = other_pos.z - (other_height * other_up.z);
    if (debug_hidden >= 0) {
        shape_draw(work, shape, &pos);
        if (debug_hidden >= 0) {
            shape_draw(other_work, other_shape, &other_pos);
        }
    }
    return 100000.0F;
}

/* the same for two capsules */
static Float collide_capsule_capsule(struct collision_work *collision,
                                     struct collision_work *other)
{
    Uint16 index = collision->current;
    Uint16 other_index = other->current;
    /* @hack spelling: the locals read in this order, which gives shape R13 and
     * height FR14, and other_shape R14 and other_height FR15 (0x8c0325b8
     * on); read as the other checks read theirs, shapes, then sizes, then
     * works, 22 of 181 words differ */
    struct collision_shape *shape = &collision->shapes[index];
    struct task_work *work = collision->owner->work;
    struct collision_shape *other_shape = &other->shapes[other_index];
    Float other_height = other_shape->size_y;
    Float height = shape->size_y;
    struct task_work *other_work = other->owner->work;
    NJS_POINT3 pos;
    NJS_POINT3 other_pos;
    NJS_POINT3 ends[2];
    NJS_POINT3 other_ends[2];
    NJS_VECTOR up;
    NJS_VECTOR other_up;

    shape_get_up(work, shape, &up);
    shape_get_center(work, shape, &pos);
    ends[0].x = pos.x + (height * up.x);
    ends[0].y = pos.y + (height * up.y);
    ends[0].z = pos.z + (height * up.z);
    ends[1].x = pos.x - (height * up.x);
    ends[1].y = pos.y - (height * up.y);
    ends[1].z = pos.z - (height * up.z);
    shape_get_up(other_work, other_shape, &other_up);
    shape_get_center(other_work, other_shape, &other_pos);
    other_ends[0].x = other_pos.x + (other_height * other_up.x);
    other_ends[0].y = other_pos.y + (other_height * other_up.y);
    other_ends[0].z = other_pos.z + (other_height * other_up.z);
    other_ends[1].x = other_pos.x - (other_height * other_up.x);
    other_ends[1].y = other_pos.y - (other_height * other_up.y);
    other_ends[1].z = other_pos.z - (other_height * other_up.z);
    if (debug_hidden >= 0) {
        shape_draw(work, shape, &pos);
        if (debug_hidden >= 0) {
            shape_draw(other_work, other_shape, &other_pos);
        }
    }
    return 100000.0F;
}
/* NOLINTEND(clang-diagnostic-unused-but-set-variable) */

/* TRUE when pos lies within other's current rectangle across x and z,
 * grown by radius on each side, and FALSE otherwise. center is the
 * rectangle's center. the rectangle turns by its task's angle, even when
 * it is SHAPE_IN_WORLD or SHAPE_UNTURNED */
static Bool rectangle_contains(struct collision_work *collision,
                               NJS_POINT3 *pos, struct collision_work *other,
                               NJS_POINT3 *center, Float radius)
{
    NJS_POINT3 corners[4];
    NJS_POINT3 turned[4];
    NJS_POINT3 point;
    NJS_VECTOR side;
    /* @hack redundant: reach, shape_radius and shape_height, set and never
     * read, which keep &turned in R8 through the first loop (0x8c03280e);
     * without them and the shape they read, 188 of 219 words differ */
    Float reach = collision->radius;
    struct task_work *work = other->owner->work;
    Uint16 index = collision->current;
    Uint16 other_index = other->current;
    struct collision_shape *shape = &collision->shapes[index];
    struct collision_shape *rectangle = &other->shapes[other_index];
    Float width = rectangle->radius;
    Float depth = rectangle->size_z;
    Float shape_radius = shape->radius;
    Float shape_height = shape->size_y;
    Uint8 i;

    point.x = pos->x;
    point.y = pos->y;
    point.z = pos->z;
    corners[0].x = radius + width;
    corners[0].y = point.y;
    corners[0].z = radius + depth;
    corners[1].x = radius + width;
    corners[1].y = point.y;
    corners[1].z = -radius - depth;
    corners[2].x = -radius - width;
    corners[2].y = point.y;
    corners[2].z = -radius - depth;
    corners[3].x = -radius - width;
    corners[3].y = point.y;
    corners[3].z = radius + depth;
    njPushMatrix(&_unit_matrix_);
    TURN(njRotateY, rectangle->ang.y + work->ang.y);
    for (i = 0; i < 4; i++) {
        njCalcPoint(NULL, &corners[i], &turned[i]);
        corners[i].x = center->x + turned[i].x;
        corners[i].y = point.y;
        corners[i].z = center->z + turned[i].z;
    }
    njPopMatrix(1);
    for (i = 0; i < 4; i++) {
        polygon_normal((const char *)&point, (const char *)&corners[i],
                       (const char *)&corners[(i + 1) % 4], &side);
        if (!(side.y > 0.0F)) {
            return FALSE;
        }
    }
    return TRUE;
}

/* pos less center, in a frame turned by ang about y */
static void point_to_local(NJS_POINT3 *pos, NJS_POINT3 *center, Angle ang,
                           NJS_POINT3 *local)
{
    NJS_POINT3 offset;
    NJS_POINT3 turned;

    offset.x = pos->x - center->x;
    offset.y = pos->y - center->y;
    offset.z = pos->z - center->z;
    njPushMatrix(&_unit_matrix_);
    TURN(njRotateY, ang);
    njCalcVector(NULL, &offset, &turned);
    njPopMatrix(1);
    local->x = turned.x;
    local->y = turned.y;
    local->z = turned.z;
}

/* the inverse of point_to_local, written back into local. unused is never
 * read */
static void point_from_local(NJS_POINT3 *unused, NJS_POINT3 *center, Angle ang,
                             NJS_POINT3 *local)
{
    NJS_MATRIX matrix;
    NJS_POINT3 turned;

    njUnitMatrix(&matrix);
    TURN_MATRIX(njRotateY, &matrix, -ang);
    njCalcVector(&matrix, local, &turned);
    local->x = turned.x + center->x;
    local->y = turned.y + center->y;
    local->z = turned.z + center->z;
}

/* a sphere against a rectangle. the two touch when the sphere overlaps the
 * rectangle's height and its extent across x and z. a touch sets
 * COLLISION_HIT_KEPT on the sphere's record and pushes nothing */
static Float collide_sphere_rectangle(struct collision_work *collision,
                                      struct collision_work *other)
{
    NJS_POINT3 pos;
    NJS_POINT3 center;
    NJS_POINT3 local;
    struct angles ang;
    struct task_work *work = collision->owner->work;
    struct task_work *other_work = other->owner->work;
    struct collision_shape *shape = &collision->shapes[collision->current];
    struct collision_shape *rectangle = &other->shapes[other->current];
    Float radius = shape->radius;
    Float width = rectangle->radius;
    Float height = rectangle->size_y;
    Float depth = rectangle->size_z;
    Float top;
    Float bottom;
    Float other_top;
    Float other_bottom;

    collision_get_shape_center(work, shape, &pos);
    shape_get_box_center(other_work, rectangle, &center);
    /* @hack spelling: the bounds set ahead of the angle, in four statements,
     * not summed in the test. they give width FR13, height FR12 and depth FR14.
     * they give other R12 and other_work R11, and other_bottom the FR5 that
     * held center.y (0x8c032a44 on); summed in the test, 116 of 153 words
     * differ and collision.c's code runs 32 bytes short */
    top = pos.y + radius;
    bottom = pos.y - radius;
    other_top = center.y + height;
    other_bottom = center.y - height;
    /* @hack redundant: ang.z, set in both arms and never read. it keeps
     * width in FR13, height in FR12 and depth in FR14. it keeps work in R9,
     * other_work in R11 and other in R12. it leaves the first arm's BRA a NOP
     * and has the second arm sum in R2; without both stores, 107 of 153 words
     * differ and collision.c's code runs 32 bytes short */
    if ((rectangle->flags & SHAPE_IN_WORLD) ||
        (rectangle->flags & SHAPE_UNTURNED)) {
        ang.y = rectangle->ang.y;
        ang.z = rectangle->ang.z;
    } else {
        ang.y = other_work->ang.y + rectangle->ang.y;
        ang.z = rectangle->ang.z;
    }
    if (top > other_bottom && bottom < other_top) {
        point_to_local(&pos, &center, -ang.y, &local);
        if (debug_hidden >= 0) {
            shape_draw(other_work, rectangle, &center);
        }
        if (-width < local.x + radius && local.x - radius < width) {
            if (-depth < local.z + radius && local.z - radius < depth) {
                collision_hit(work, collision, other_work, other);
                collision_hurt(collision, other);
                collision->flags |= COLLISION_HIT_KEPT;
            }
        }
    }
    return 100.0F;
}

/* a shape against a rectangle: land on its top, bump its bottom, or be
 * pushed out of its side */
static void collide_land_rectangle(struct collision_work *collision,
                                   struct collision_work *other)
{
    NJS_POINT3 pos;
    NJS_POINT3 center;
    struct collision_shape *shape = &collision->shapes[collision->current];
    struct collision_shape *rectangle = &other->shapes[other->current];
    Float radius = shape->radius;
    Float height = rectangle->size_y;
    /* @hack redundant: width and depth, set and never read, which give
     * rectangle R11 and other_work R10 (0x8c032be6 on); without them, 142 of
     * 287 words differ */
    Float width = rectangle->radius;
    Float depth = rectangle->size_z;
    struct task_work *work = collision->owner->work;
    struct task_work *other_work = other->owner->work;
    Float bottom;
    Float top;
    Float other_bottom;
    Float other_top;

    collision_get_shape_center(work, shape, &pos);
    shape_get_box_center(other_work, rectangle, &center);
    /* @hack spelling: each bound set from its center, then moved by the size,
     * which gives other_top FR15, radius FR14, top FR13 and bottom FR12, and
     * keeps other_bottom in the frame (0x8c032bd8 on); set by sums, 167 of
     * 287 words differ */
    top = pos.y;
    top += radius;
    bottom = pos.y;
    bottom -= radius;
    other_top = center.y;
    other_bottom = center.y - height;
    other_top += height;
    if (debug_hidden >= 0) {
        shape_draw(other_work, rectangle, &center);
    }
    if (bottom < other_top && bottom > other_top - LAND_DEPTH &&
        rectangle_contains(collision, &pos, other, &center, radius)) {
        if (debug_hidden >= 0) {
            njPrintC(NJM_LOCATION(10, 15), "UP");
        }
        if (!(collision->flags & COLLISION_HIT_KEPT)) {
            collision_hit(work, collision, other_work, other);
        }
        collision_hurt(collision, other);
        work->flags |= COLLISION_ON_TOP;
        other_work->flags |= COLLISION_UNDER;
        collision->flags |= COLLISION_HIT_BOTTOM;
        other->flags |= COLLISION_HIT_TOP;
        if (try_mark_pushed(collision, shape, other, rectangle)) {
            work->pos.y = other_top - 0.01F;
            work->ang.x = 0;
            work->ang.z = 0;
        }
    } else if (rectangle_contains(collision, &pos, other, &center, radius)) {
        if (top > other_bottom && top < other_bottom + 2.0F) {
            collision_hit(work, collision, other_work, other);
            collision_hurt(collision, other);
            work->flags |= COLLISION_UNDER;
            other_work->flags |= COLLISION_ON_TOP;
            collision->flags |= COLLISION_HIT_TOP;
            other->flags |= COLLISION_HIT_BOTTOM;
            if (try_mark_pushed(collision, shape, other, rectangle)) {
                work->pos.y = other_bottom - (radius * 2.0F) + 0.01F;
                /* @hack redundant: a store to work->pos.y that never runs, with
                 * which SHC copies radius to FR2 for the product and loads
                 * other_bottom after it (0x8c032da8 to 0x8c032db4); without it,
                 * 19 of 287 words differ */
                if (0) {
                    work->pos.y = other_top + other_top - radius;
                }
            }
            if (debug_hidden >= 0) {
                njPrintC(NJM_LOCATION(10, 15), "DOWN");
            }
        } else if (bottom < other_top - LAND_DEPTH && top > other_bottom) {
            collision_hit(work, collision, other_work, other);
            collision_hurt(collision, other);
            collision->flags |= COLLISION_HIT_KEPT;
            collision->flags |= COLLISION_HIT_SIDE;
            other->flags |= COLLISION_HIT_SIDE;
            if (try_mark_pushed(collision, shape, other, rectangle)) {
                rectangle_push_out(collision, &pos, other, &center);
            }
            if (debug_hidden >= 0) {
                njPrintC(NJM_LOCATION(10, 16), "SIDE");
            }
        }
    }
}

/* the same for a player, swept from where it was a step before so as not to
 * pass through the rectangle. the sweep stops at the first point that
 * touches, and writes the task's position then as the player's latest place
 * in its history */
static void collide_sweep_rectangle(struct collision_work *collision,
                                    struct collision_work *other)
{
    NJS_POINT3 pos;
    NJS_POINT3 center;
    NJS_POINT3 last;
    NJS_POINT3 stride;
    struct collision_shape *shape = &collision->shapes[collision->current];
    struct collision_shape *rectangle = &other->shapes[other->current];
    Float radius = shape->radius;
    Float height = rectangle->size_y;
    /* @hack redundant: width, set and never read, which keeps distance and
     * height from trading FR13 and FR14 (0x8c032f2e on); without it, 14 of
     * 482 words differ */
    Float width = rectangle->radius;
    struct task_work *work = collision->owner->work;
    struct task_work *other_work = other->owner->work;
    Float distance;
    int count;
    Float bottom;
    Float top;
    Float other_bottom;
    Float other_top;

    player_get_history(PLAYER_VARS(work)->player, 1, &last, NULL);
    stride.x = work->pos.x - last.x;
    stride.y = work->pos.y - last.y;
    stride.z = work->pos.z - last.z;
    distance = njSqrt((stride.x * stride.x) + (stride.y * stride.y) +
                      (stride.z * stride.z));
    if (distance > 0.0F) {
        stride.x /= distance;
        stride.y /= distance;
        stride.z /= distance;
        stride.x *= SWEEP_STRIDE;
        stride.y *= SWEEP_STRIDE;
        stride.z *= SWEEP_STRIDE;
    } else {
        distance = 0.01F;
        stride.x = 0.0F;
        stride.y = 0.0F;
        stride.z = 0.0F;
    }
    count = (int)ceilf(distance * (1.0F / SWEEP_STRIDE));
    pos.x = last.x;
    pos.y = last.y + radius;
    pos.z = last.z;
    shape_get_box_center(other_work, rectangle, &center);
    other_top = center.y + height;
    other_bottom = center.y - height;
    if (debug_hidden >= 0) {
        shape_draw(other_work, rectangle, &center);
    }
    while (count > 0) {
        count--;
        if (count > 0) {
            pos.x += stride.x;
            pos.y += stride.y;
            pos.z += stride.z;
        } else {
            pos.x = work->pos.x;
            pos.y = work->pos.y + radius;
            pos.z = work->pos.z;
        }
        bottom = pos.y - radius;
        top = pos.y + radius;
        if (bottom < other_top && bottom > other_top - LAND_DEPTH &&
            rectangle_contains(collision, &pos, other, &center, radius)) {
            if (debug_hidden >= 0) {
                njPrintC(NJM_LOCATION(10, 15), "UP");
            }
            if (!(collision->flags & COLLISION_HIT_KEPT)) {
                collision_hit(work, collision, other_work, other);
            }
            collision_hurt(collision, other);
            other_work->flags |= COLLISION_UNDER;
            collision->flags |= COLLISION_HIT_BOTTOM;
            other->flags |= COLLISION_HIT_TOP;
            if (try_mark_pushed(collision, shape, other, rectangle)) {
                work->pos.y = other_top - 0.01F;
                work->ang.x = 0;
                work->ang.z = 0;
                work->flags |= COLLISION_ON_TOP;
                work->flags &= ~COLLISION_TOUCHING;
            }
            player_set_history(PLAYER_VARS(collision->owner->work)->player, 0,
                               &work->pos, NULL);
            break;
        } else if (rectangle_contains(collision, &pos, other, &center,
                                      radius)) {
            if (top > other_bottom && top < other_bottom + 2.0F) {
                collision_hit(work, collision, other_work, other);
                collision_hurt(collision, other);
                other_work->flags |= COLLISION_ON_TOP;
                collision->flags |= COLLISION_HIT_TOP;
                other->flags |= COLLISION_HIT_BOTTOM;
                if (try_mark_pushed(collision, shape, other, rectangle)) {
                    work->pos.y = other_bottom - (radius * 2.0F) + 0.01F;
                    work->flags &= ~COLLISION_TOUCHING;
                    work->flags |= COLLISION_UNDER;
                }
                if (debug_hidden >= 0) {
                    njPrintC(NJM_LOCATION(10, 15), "DOWN");
                }
                player_set_history(PLAYER_VARS(collision->owner->work)->player,
                                   0, &work->pos, NULL);
                break;
            } else if (bottom < other_top - LAND_DEPTH && top > other_bottom) {
                collision_hit(work, collision, other_work, other);
                collision_hurt(collision, other);
                collision->flags |= COLLISION_HIT_KEPT;
                collision->flags |= COLLISION_HIT_SIDE;
                other->flags |= COLLISION_HIT_SIDE;
                if (try_mark_pushed(collision, shape, other, rectangle)) {
                    rectangle_push_out(collision, &pos, other, &center);
                }
                if (debug_hidden >= 0) {
                    njPrintC(NJM_LOCATION(10, 15), "SIDE");
                }
                player_set_history(PLAYER_VARS(collision->owner->work)->player,
                                   0, &work->pos, NULL);
                break;
            } else {
                /* @hack redundant: an empty else, which has the loop's test
                 * load count into R2, not R3 (0x8c03333e); without it, 2 of 482
                 * words differ */
            }
        }
    }
}

/* the sweep against a rectangle2: land on its upper face, bump into its
 * lower one, or be pushed out of the side it's nearest. the tilt counts
 * from its nearest quarter turn, and what is left of it still tilts the
 * faces. past an eighth of a turn, the rectangle2 lies on its side or
 * upside down. on its side, its width and height trade */
static void collide_sweep_rectangle2(struct collision_work *collision,
                                     struct collision_work *other)
{
    NJS_POINT3 pos;
    NJS_POINT3 center;
    struct angles ang;
    NJS_POINT3 last;
    NJS_POINT3 stride;
    NJS_POINT3 local;
    NJS_POINT3 left;
    NJS_POINT3 right;
    NJS_POINT3 corner;
    NJS_POINT3 turned;
    NJS_POINT3 out;
    struct collision_shape *shape = &collision->shapes[collision->current];
    struct collision_shape *rectangle = &other->shapes[other->current];
    Float radius = shape->radius;
    Float width = rectangle->radius;
    Float height = shape->size_y;
    Float other_height = rectangle->size_y;
    Float depth = shape->size_z;
    Float other_depth = rectangle->size_z;
    struct task_work *work = collision->owner->work;
    struct task_work *other_work = other->owner->work;
    Float distance;
    Float bottom;
    Float top;
    Float upper;
    Float lower;
    Float side1;
    Float side2;
    Float along;
    Float across;
    int count;

    player_get_history(PLAYER_VARS(work)->player, 1, &last, NULL);
    stride.x = work->pos.x - last.x;
    stride.y = work->pos.y - last.y;
    stride.z = work->pos.z - last.z;
    distance = njSqrt((stride.x * stride.x) + (stride.y * stride.y) +
                      (stride.z * stride.z));
    if (distance > 0.0F) {
        stride.x /= distance;
        stride.y /= distance;
        stride.z /= distance;
        stride.x *= SWEEP_STRIDE;
        stride.y *= SWEEP_STRIDE;
        stride.z *= SWEEP_STRIDE;
    } else {
        distance = 0.01F;
        stride.x = 0.0F;
        stride.y = 0.0F;
        stride.z = 0.0F;
    }
    count = (int)ceilf(distance * (1.0F / SWEEP_STRIDE));
    pos.x = last.x;
    pos.y = last.y + height;
    pos.z = last.z;
    shape_get_box_center(other_work, rectangle, &center);
    if (debug_hidden >= 0) {
        shape_draw(other_work, rectangle, &center);
    }
    if ((rectangle->flags & SHAPE_IN_WORLD) ||
        (rectangle->flags & SHAPE_UNTURNED)) {
        ang.y = rectangle->ang.y;
        ang.z = rectangle->ang.z;
    } else {
        ang.y = other_work->ang.y + rectangle->ang.y;
        ang.z = rectangle->ang.z;
    }
    /* @hack spelling: ang.x, a field this check has no other use for, holds the
     * tilt plus an eighth of a turn. it keeps width in FR12 and radius in
     * FR14 (0x8c0333f8, 0x8c0333fe); in a local of its own, the two trade and
     * 84 of 981 words differ */
    ang.x = (Uint16)(ang.z + NJM_DEG_ANG(45));
    if (ang.x >= NJM_DEG_ANG(90) && ang.x < NJM_DEG_ANG(180)) {
        width = rectangle->size_y;
        other_height = rectangle->radius;
        ang.z = ang.z - NJM_DEG_ANG(90);
    } else if (ang.x >= NJM_DEG_ANG(180) && ang.x < NJM_DEG_ANG(270)) {
        ang.z = ang.z - NJM_DEG_ANG(180);
    } else if (ang.x >= NJM_DEG_ANG(270) && ang.x <= 0xffff) {
        width = rectangle->size_y;
        other_height = rectangle->radius;
        ang.z = ang.z - NJM_DEG_ANG(270);
    }
    while (count > 0) {
        count--;
        if (count > 0) {
            pos.x += stride.x;
            pos.y += stride.y;
            pos.z += stride.z;
        } else {
            pos.x = work->pos.x;
            pos.y = work->pos.y + height;
            pos.z = work->pos.z;
        }
        point_to_local(&pos, &center, -ang.y, &local);
        if (-radius - other_depth < local.z && local.z < radius + other_depth) {
            njPushMatrix(&_unit_matrix_);
            TURN(njRotateZ, ang.z);
            corner.x = -width - depth;
            corner.y = other_height;
            corner.z = 0.0F;
            njCalcPoint(NULL, &corner, &left);
            corner.x = width + depth;
            corner.y = other_height;
            corner.z = 0.0F;
            njCalcPoint(NULL, &corner, &right);
            njPopMatrix(1);
            top = local.y + height;
            bottom = local.y - height;
            upper = (njTan(ang.z) * (local.x - left.x)) + left.y;
            lower = (njTan(ang.z) * (local.x + left.x)) - left.y;
            if (-depth - other_depth < local.z &&
                local.z < depth + other_depth) {
                if (left.x < local.x && local.x < right.x) {
                    if (upper - 3.0F < bottom && bottom <= upper) {
                        if (debug_hidden >= 0) {
                            njPrintC(NJM_LOCATION(20, 15),
                                     "RECTANGLE2 UPPER!!");
                        }
                        if (!(collision->flags & COLLISION_HIT_KEPT)) {
                            collision_hit(work, collision, other_work, other);
                        }
                        collision_hurt(collision, other);
                        other_work->flags |= COLLISION_UNDER;
                        collision->flags |= COLLISION_HIT_BOTTOM;
                        other->flags |= COLLISION_HIT_TOP;
                        if (try_mark_pushed(collision, shape, other,
                                            rectangle)) {
                            work->pos.y += upper - local.y + height;
                            work->ang.x = 0;
                            work->ang.z = 0;
                            work->flags &= ~COLLISION_TOUCHING;
                            work->flags |= COLLISION_ON_TOP;
                        }
                        player_set_history(
                            PLAYER_VARS(collision->owner->work)->player, 0,
                            &work->pos, NULL);
                        break;
                    }
                }
            }
            if (-right.x < local.x && local.x < -left.x && lower <= top &&
                top <= lower + 3.0F) {
                if (debug_hidden >= 0) {
                    njPrintC(NJM_LOCATION(20, 15), "RECTANGLE2 LOWER!!");
                }
                if (!(collision->flags & COLLISION_HIT_KEPT)) {
                    collision_hit(work, collision, other_work, other);
                }
                collision_hurt(collision, other);
                other_work->flags |= COLLISION_ON_TOP;
                collision->flags |= COLLISION_HIT_TOP;
                other->flags |= COLLISION_HIT_BOTTOM;
                if (try_mark_pushed(collision, shape, other, rectangle)) {
                    if (!(work->flags & PLAYER_ON_LAND)) {
                        work->pos.y += lower - local.y - height;
                    }
                    work->flags &= ~COLLISION_TOUCHING;
                    work->flags |= COLLISION_UNDER;
                }
                player_set_history(PLAYER_VARS(collision->owner->work)->player,
                                   0, &work->pos, NULL);
                break;
            }
            njPushMatrix(&_unit_matrix_);
            TURN(njRotateZ, -ang.z);
            njCalcVector(NULL, &local, &turned);
            njPopMatrix(1);
            if (turned.x > -radius - width && turned.x < radius + width &&
                turned.y < radius + other_height - 4.0F &&
                turned.y > 2.0F - radius - other_height) {
                collision_hit(work, collision, other_work, other);
                collision_hurt(collision, other);
                collision->flags |= COLLISION_HIT_KEPT;
                collision->flags |= COLLISION_HIT_SIDE;
                other->flags |= COLLISION_HIT_SIDE;
                if (try_mark_pushed(collision, shape, other, rectangle)) {
                    along = (radius + width) * turned.z;
                    across = (radius + other_depth) * turned.x;
                    side1 = along - across;
                    side2 = along + across;
                    if (debug_hidden >= 0) {
                        njPrintC(NJM_LOCATION(20, 15), "RECTANGLE2 SIDE!!");
                    }
                    if (side1 > 0.0F) {
                        if (side2 > 0.0F) {
                            out.x = 0.0F;
                            out.y = 0.0F;
                            out.z =
                                ((radius + other_depth) * 0.999F) - turned.z;
                            if (debug_hidden >= 0) {
                                njPrintC(NJM_LOCATION(20, 16), "FACE 1");
                            }
                        } else {
                            out.x = ((-radius - width) * 0.999F) - turned.x;
                            out.y = 0.0F;
                            out.z = 0.0F;
                            if (debug_hidden >= 0) {
                                njPrintC(NJM_LOCATION(20, 16), "FACE 2");
                            }
                        }
                    } else if (side2 < 0.0F) {
                        out.x = 0.0F;
                        out.y = 0.0F;
                        out.z = ((-radius - other_depth) * 0.999F) - turned.z;
                        if (debug_hidden >= 0) {
                            njPrintC(NJM_LOCATION(20, 16), "FACE 3");
                        }
                    } else {
                        out.x = ((radius + width) * 0.999F) - turned.x;
                        out.y = 0.0F;
                        out.z = 0.0F;
                        if (debug_hidden >= 0) {
                            njPrintC(NJM_LOCATION(20, 16), "FACE 4");
                        }
                    }
                    njPushMatrix(&_unit_matrix_);
                    TURN(njRotateZ, ang.z);
                    TURN(njRotateY, ang.y);
                    njCalcVector(NULL, &out, &local);
                    njPopMatrix(1);
                    work->pos.x += local.x * 0.3F;
                    if (!(work->flags & PLAYER_ON_LAND)) {
                        work->pos.y += local.y * 0.3F;
                    }
                    work->pos.z += local.z * 0.3F;
                }
                player_set_history(PLAYER_VARS(collision->owner->work)->player,
                                   0, &work->pos, NULL);
                break;
            } else {
                /* @hack redundant: an empty else, which has the loop's test
                 * load count into R2, not R3 (0x8c033d68); without it, 2 of 981
                 * words differ */
            }
        }
    }
}

/* the sweep against a wall, which pushes the player back from its face or
 * puts it back in front, as WALL_REACH says. only a player within the
 * wall's extent across its x and y counts */
static void collide_sweep_wall(struct collision_work *collision,
                               struct collision_work *other)
{
    NJS_POINT3 pos;
    NJS_POINT3 center;
    struct angles ang;
    NJS_POINT3 last;
    NJS_POINT3 stride;
    NJS_POINT3 local;
    struct collision_shape *shape = &collision->shapes[collision->current];
    struct collision_shape *wall = &other->shapes[other->current];
    Float radius = shape->radius;
    Float width = wall->radius;
    Float height = shape->size_y;
    Float other_height = wall->size_y;
    /* @hack redundant: reach, set and never read, which keeps the hoisted
     * -other_height at R15+8 and -width at R15+16 (0x8c033f88, 0x8c033f90);
     * without it, 4 of 384 words differ */
    Float reach = other->radius;
    /* @hack spelling: other_work read ahead of work, which puts the hoisted
     * radius + 0.1F at R15+12 (0x8c033f9a); read after it, 4 of 384 words
     * differ */
    struct task_work *other_work = other->owner->work;
    struct task_work *work = collision->owner->work;
    Float distance;
    int count;

    player_get_history(PLAYER_VARS(work)->player, 1, &last, NULL);
    stride.x = work->pos.x - last.x;
    stride.y = work->pos.y - last.y;
    stride.z = work->pos.z - last.z;
    if ((wall->flags & SHAPE_IN_WORLD) || (wall->flags & SHAPE_UNTURNED)) {
        ang.y = wall->ang.y;
    } else {
        ang.y = other_work->ang.y + wall->ang.y;
    }
    distance = njSqrt((stride.x * stride.x) + (stride.y * stride.y) +
                      (stride.z * stride.z));
    if (distance > 0.0F) {
        stride.x /= distance;
        stride.y /= distance;
        stride.z /= distance;
        stride.x *= SWEEP_STRIDE;
        stride.y *= SWEEP_STRIDE;
        stride.z *= SWEEP_STRIDE;
    } else {
        distance = 0.01F;
        stride.x = 0.0F;
        stride.y = 0.0F;
        stride.z = 0.0F;
    }
    count = (int)ceilf(distance * (1.0F / SWEEP_STRIDE));
    pos.x = last.x;
    pos.y = last.y + height;
    pos.z = last.z;
    shape_get_box_center(other_work, wall, &center);
    if (debug_hidden >= 0) {
        shape_draw(other_work, wall, &center);
    }
    while (count >= 0) {
        count--;
        if (count > 0) {
            pos.x += stride.x;
            pos.y += stride.y;
            pos.z += stride.z;
        } else {
            pos.x = work->pos.x;
            pos.y = work->pos.y + height;
            pos.z = work->pos.z;
        }
        point_to_local(&pos, &center, -ang.y, &local);
        if (local.y + height > -other_height &&
            local.y - height < other_height && local.x + radius > -width &&
            local.x - radius < width) {
            distance = local.z;
            if (distance - radius < WALL_REACH && distance - radius >= 0.0F) {
                local.z += (radius - distance + WALL_REACH) * WALL_PUSH;
                point_from_local(&pos, &center, -ang.y, &local);
                local.y -= height;
                work->pos.x = local.x;
                work->pos.z = local.z;
                break;
            }
            distance = local.z;
            if (distance - radius < 0.0F && distance > -WALL_REACH) {
                local.z = radius + 0.1F;
                point_from_local(&pos, &center, -ang.y, &local);
                local.y -= height;
                work->pos.x = local.x;
                work->pos.z = local.z;
                player_set_history(PLAYER_VARS(collision->owner->work)->player,
                                   0, &work->pos, NULL);
                break;
            }
        }
    }
}

/* a player's first shape against a bound: pull it back inside the bound's
 * circle, across x and z */
static void collide_bound(struct collision_work *collision,
                          struct collision_work *other)
{
    NJS_POINT3 pos;
    NJS_POINT3 center;
    /* @hack redundant: 12 bytes of frame that no code reads, of a type unknown,
     * declared as a Uint32[3]. the image's ADD #-52,R15 reserves them
     * (0x8c03416e); without them, 7 of 154 words differ */
    Uint32 unused[3];
    NJS_POINT3 offset;
    struct collision_shape *shape = &collision->shapes[collision->current];
    struct collision_shape *bound = &other->shapes[other->current];
    Float height = shape->size_y;
    /* @hack redundant: width, other_height, depth and ang, set from bound
     * and never read, which give bound R14 and work R13 (0x8c034172 on);
     * without them, 39 of 154 words differ */
    Float width = bound->radius;
    Float other_height = bound->size_y;
    Float depth = bound->size_z;
    Angle ang = bound->ang.y;
    struct task_work *work = collision->owner->work;
    struct task_work *other_work = other->owner->work;
    Float out;

    pos = work->pos;
    shape_get_box_center(other_work, bound, &center);
    if (debug_hidden >= 0) {
        shape_draw(other_work, bound, &center);
    }
    pos.x = work->pos.x;
    pos.y = work->pos.y + height;
    pos.z = work->pos.z;
    offset.x = pos.x - center.x;
    offset.y = 0.0F;
    offset.z = pos.z - center.z;
    out = njScalor(&offset);
    out += shape->radius;
    out -= bound->radius;
    if (out > 0.0F) {
        if (njScalor(&offset) == 0.0F) {
            offset.x = offset.y = 0.0F;
            offset.z = 0.1F;
        } else {
            njUnitVector(&offset);
        }
        out = -0.1F - out;
        offset.x = offset.x * out * 0.7F;
        offset.z = offset.z * out * 0.7F;
        work->pos.x += offset.x;
        work->pos.z += offset.z;
    }
}

/* TRUE when either current shape's SHAPE_IGNORE_* flags name the other
 * record's kind, and FALSE otherwise */
static Bool collision_ignores(struct collision_work *collision,
                              struct collision_work *other)
{
    struct collision_shape *shape = &collision->shapes[collision->current];
    struct collision_shape *other_shape = &other->shapes[other->current];

    switch (collision->kind) {
    case COLLISION_PLAYER:
    case COLLISION_ATTACK:
        if (other_shape->flags & SHAPE_IGNORE_PLAYERS) {
            return TRUE;
        }
        break;
    case COLLISION_ITEM:
    case COLLISION_ENEMY:
        if (other_shape->flags & SHAPE_IGNORE_ENEMIES) {
            return TRUE;
        }
        break;
    case COLLISION_OBJECT:
    case COLLISION_OBJECT_MUTUAL:
        if (other_shape->flags & SHAPE_IGNORE_OBJECTS) {
            return TRUE;
        }
        break;
    case COLLISION_PICKUP:
    case COLLISION_RING:
        if (other_shape->flags & SHAPE_IGNORE_OTHERS) {
            return TRUE;
        }
        break;
    }
    switch (other->kind) {
    case COLLISION_PLAYER:
    case COLLISION_ATTACK:
        if (shape->flags & SHAPE_IGNORE_PLAYERS) {
            return TRUE;
        }
        break;
    case COLLISION_ITEM:
    case COLLISION_ENEMY:
        if (shape->flags & SHAPE_IGNORE_ENEMIES) {
            return TRUE;
        }
        break;
    case COLLISION_OBJECT:
    case COLLISION_OBJECT_MUTUAL:
        if (shape->flags & SHAPE_IGNORE_OBJECTS) {
            return TRUE;
        }
        break;
    case COLLISION_PICKUP:
    case COLLISION_RING:
        if (shape->flags & SHAPE_IGNORE_OTHERS) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

/* when collision is a player's record, list other near that player if it
 * is a target within NEAR_REACH, by the rules beside near_tasks. distance is
 * squared */
static void collision_list_near(struct collision_work *collision,
                                struct collision_work *other, Float distance)
{
    struct near_task *near;

    if (collision->kind != COLLISION_PLAYER) {
        return;
    }
    if (collision->owner->work == player_work[0] &&
        other->owner->work == player_work[1]) {
        if (collision->flags & COLLISION_TARGET) {
            if (distance < NEAR_REACH && near_task_count_player1 < NEAR_TASKS) {
                near = &near_tasks_player1[near_task_count_player1];
                near->work = collision->owner->work;
                near->distance = distance;
                near++;
                near->work = NULL;
                near_task_count_player1++;
            }
        }
    }
    if (other->flags & COLLISION_TARGET) {
        if (collision->owner->work == player_work[0]) {
            if (other->owner->work->id == RING_ID) {
                if (distance < NEAR_REACH && near_ring_count < NEAR_RINGS) {
                    near = &near_rings[near_ring_count];
                    near->work = other->owner->work;
                    near->distance = distance;
                    near++;
                    near->work = NULL;
                    near_ring_count++;
                }
            } else if (distance < NEAR_REACH && near_task_count < NEAR_TASKS) {
                near = &near_tasks[near_task_count];
                near->work = other->owner->work;
                near->distance = distance;
                near++;
                near->work = NULL;
                near_task_count++;
            }
        } else if (collision->owner->work == player_work[1]) {
            if (other->owner->work->id == RING_ID) {
                if (distance < NEAR_REACH &&
                    near_ring_count_player1 < NEAR_RINGS) {
                    near = &near_rings_player1[near_ring_count_player1];
                    near->work = other->owner->work;
                    near->distance = distance;
                    near++;
                    near->work = NULL;
                    near_ring_count_player1++;
                }
            } else if (distance < NEAR_REACH &&
                       near_task_count_player1 < NEAR_TASKS) {
                near = &near_tasks_player1[near_task_count_player1];
                near->work = other->owner->work;
                near->distance = distance;
                near++;
                near->work = NULL;
                near_task_count_player1++;
            }
        }
    }
}

/* TRUE when the two records' radii together reach across the distance
 * between their tasks' positions, and FALSE otherwise. it lists the pair
 * with collision_list_near either way */
static Bool list_near_and_test_reach(struct task_work *work,
                                     struct collision_work *collision,
                                     struct task_work *other_work,
                                     struct collision_work *other)
{
    Float radius = collision->radius;
    Float other_radius = other->radius;
    /* @hack spelling: from and to, Float cursors stepped across the members
     * of the two tasks' pos; as work->pos's and other_work->pos's members, 26
     * of 47 words differ */
    /* @hack spelling: from declared ahead of reach and limit, which gives from
     * R4 and to R5, and keeps dx in FR5 and dz in FR6 (0x8c0345e2 to
     * 0x8c0345ea, 0x8c03460e); declared after them with to, 39 of 47 words
     * differ */
    Float *from = &work->pos.x;
    Float reach = radius + other_radius;
    Float limit = reach * reach;
    /* @hack spelling: to declared after limit, which loads each part of to
     * after from's (FMOV.S @R5+,FR3 at 0x8c0345fc); declared with from, 8 of 47
     * words differ */
    Float *to = &other_work->pos.x;
    Float dx;
    Float dy;
    Float dz;
    Float distance;
    Float to_y;
    Float from_z;
    Float to_z;

    /* @hack spelling: the parts read a statement at a time, which gives from_z
     * FR8 and to_z FR9 (0x8c034604, 0x8c034608); read in the differences,
     * 24 of 47 words differ */
    dx = *from++ - *to++;
    dy = *from++;
    to_y = *to++;
    from_z = *from;
    to_z = *to;
    /* @hack spelling: two statements joined by a comma, which gives to_y FR7
     * (FMOV.S @R5+,FR7 at 0x8c034600); as two, 6 of 47 words differ */
    dy -= to_y, dz = from_z - to_z;
    distance = (dx * dx) + (dy * dy) + (dz * dz);

    collision_list_near(collision, other, distance);
    return distance < limit;
}

/* each check of two records tests every shape of one against every shape
 * of the other, where the records reach each other. the kinds of the two
 * shapes pick the test. where the test takes the shapes in the other order,
 * the check passes it the records the other way round */
/* NOLINTBEGIN(readability-suspicious-call-argument) */

/* the check for most pairs, between spheres, cylinders, capsules and
 * rectangles */
static Float collide_pair(struct collision_work *collision,
                          struct collision_work *other)
{
    Float result = 1000000.0F;
    Uint16 i;
    Uint16 j;
    struct task_work *work = collision->owner->work;

    if (list_near_and_test_reach(work, collision, other->owner->work, other)) {
        for (i = 0; i < collision->count; i++) {
            for (j = 0; j < other->count; j++) {
                collision->current = i;
                other->current = j;
                if (collision_ignores(collision, other)) {
                    continue;
                }
                /* @hack redundant: an else after this if's continue, which has
                 * the kind tests load the shape's address into R2 and its flags
                 * into R3 (MOV.L @(12,R14),R2 at 0x8c0346e4); without it, 4
                 * of 230 words differ */
                if (collision->shapes[i].flags & SHAPE_OFF ||
                    other->shapes[j].flags & SHAPE_OFF) {
                    continue;
                } else if (!(collision->shapes[i].flags & SHAPE_KIND)) {
                    switch (other->shapes[j].flags & SHAPE_KIND) {
                    case SHAPE_SPHERE:
                        collide_sphere_sphere(collision, other);
                        break;
                    case SHAPE_CYLINDER:
                        collide_sphere_cylinder(collision, other);
                        break;
                    case SHAPE_CAPSULE:
                        collide_sphere_capsule(collision, other);
                        break;
                    case SHAPE_RECTANGLE:
                        collide_sphere_rectangle(collision, other);
                        break;
                    }
                } else if ((collision->shapes[i].flags & SHAPE_KIND) ==
                           SHAPE_CYLINDER) {
                    switch (other->shapes[j].flags & SHAPE_KIND) {
                    case SHAPE_SPHERE:
                        result = collide_sphere_cylinder(other, collision);
                        break;
                    case SHAPE_CYLINDER:
                        result = collide_cylinder_cylinder(collision, other);
                        break;
                    case SHAPE_CAPSULE:
                        result = collide_cylinder_capsule(collision, other);
                        break;
                    case SHAPE_RECTANGLE:
                        break;
                    }
                } else if ((collision->shapes[i].flags & SHAPE_KIND) ==
                           SHAPE_CAPSULE) {
                    switch (other->shapes[j].flags & SHAPE_KIND) {
                    case SHAPE_SPHERE:
                        result = collide_sphere_capsule(other, collision);
                        break;
                    case SHAPE_CYLINDER:
                        result = collide_cylinder_capsule(other, collision);
                        break;
                    case SHAPE_CAPSULE:
                        result = collide_capsule_capsule(collision, other);
                        break;
                    case SHAPE_RECTANGLE:
                        break;
                    }
                } else if ((collision->shapes[i].flags & SHAPE_KIND) ==
                           SHAPE_RECTANGLE) {
                    switch (other->shapes[j].flags & SHAPE_KIND) {
                    case SHAPE_SPHERE:
                        result = collide_sphere_rectangle(other, collision);
                        break;
                    case SHAPE_CYLINDER:
                        break;
                    case SHAPE_RECTANGLE:
                        break;
                    /* @hack redundant: a default that only breaks, which has
                     * the loop's test extend j into R2 (EXTU.W R10,R2 at
                     * 0x8c034828); without it, 2 of 230 words differ */
                    default:
                        break;
                    }
                }
            }
        }
    }
    return result;
}

/* the pair check against items and enemies. it differs from
 * collide_pair in two tests: a sphere of collision's meets a
 * cylinder as a cylinder does, and a rectangle of collision's meets
 * nothing */
static Float collide_target(struct collision_work *collision,
                            struct collision_work *other)
{
    Float result = 1000000.0F;
    Uint16 i;
    Uint16 j;
    struct task_work *work = collision->owner->work;

    if (list_near_and_test_reach(work, collision, other->owner->work, other)) {
        for (i = 0; i < collision->count; i++) {
            for (j = 0; j < other->count; j++) {
                collision->current = i;
                other->current = j;
                if (collision_ignores(collision, other)) {
                    continue;
                }
                /* @hack redundant: an else after this if's continue, as in
                 * collide_pair (0x8c034904); without it, 4 of 220
                 * words differ */
                if (collision->shapes[i].flags & SHAPE_OFF ||
                    other->shapes[j].flags & SHAPE_OFF) {
                    continue;
                } else if (!(collision->shapes[i].flags & SHAPE_KIND)) {
                    switch (other->shapes[j].flags & SHAPE_KIND) {
                    case SHAPE_SPHERE:
                        result = collide_sphere_sphere(collision, other);
                        break;
                    case SHAPE_CYLINDER:
                        result = collide_cylinder_cylinder(collision, other);
                        break;
                    case SHAPE_CAPSULE:
                        result = collide_sphere_capsule(collision, other);
                        break;
                    case SHAPE_RECTANGLE:
                        collide_sphere_rectangle(collision, other);
                        break;
                    }
                } else if ((collision->shapes[i].flags & SHAPE_KIND) ==
                           SHAPE_CYLINDER) {
                    switch (other->shapes[j].flags & SHAPE_KIND) {
                    case SHAPE_SPHERE:
                        result = collide_sphere_cylinder(other, collision);
                        break;
                    case SHAPE_CYLINDER:
                        result = collide_cylinder_cylinder(collision, other);
                        break;
                    case SHAPE_CAPSULE:
                        result = collide_cylinder_capsule(collision, other);
                        break;
                    case SHAPE_RECTANGLE:
                        break;
                    }
                } else if ((collision->shapes[i].flags & SHAPE_KIND) ==
                           SHAPE_CAPSULE) {
                    switch (other->shapes[j].flags & SHAPE_KIND) {
                    case SHAPE_SPHERE:
                        result = collide_sphere_capsule(other, collision);
                        break;
                    case SHAPE_CYLINDER:
                        result = collide_cylinder_capsule(other, collision);
                        break;
                    case SHAPE_CAPSULE:
                        result = collide_capsule_capsule(collision, other);
                        break;
                    case SHAPE_RECTANGLE:
                        break;
                    }
                } else if ((collision->shapes[i].flags & SHAPE_KIND) ==
                           SHAPE_RECTANGLE) {
                    switch (other->shapes[j].flags & SHAPE_KIND) {
                    case SHAPE_SPHERE:
                        break;
                    case SHAPE_CYLINDER:
                        break;
                    case SHAPE_RECTANGLE:
                        break;
                    /* @hack redundant: a default that only breaks, which has
                     * the loop's test extend j into R3 (EXTU.W R10,R3 at
                     * 0x8c034a38); without it, 2 of 220 words differ */
                    default:
                        break;
                    }
                }
            }
        }
    }
    return result;
}

/* a player against an object. the player's first shape meets every kind
 * but walls and bounds, and sweeps against both kinds of rectangle. its
 * other shapes meet the object's as collide_pair's do, but for its
 * rectangles, which meet nothing */
static void collide_object(struct collision_work *collision,
                           struct collision_work *other)
{
    struct collision_shape *shapes = collision->shapes;
    struct collision_shape *other_shapes = other->shapes;
    struct task_work *work;
    Uint16 i;
    Uint16 j;

    if (shapes == NULL) {
        return;
    }
    if (other_shapes == NULL) {
        return;
    }
    /* @hack spelling: work set after the null tests, which fills the call's
     * delay slot with MOV R13,R5 (0x8c034aa0) and keeps the second loop's i
     * in the frame; set at its declaration, 17 of 312 words differ */
    work = collision->owner->work;
    if (!list_near_and_test_reach(work, collision, other->owner->work, other)) {
        return;
    }
    /* @hack spelling: i set after the tests, where SHC zeroes its R8 (MOV #0,R8
     * at 0x8c034aaa); set at its declaration, 36 of 312 words differ */
    i = 0;
    for (j = 0; j < other->count; j++) {
        collision->current = i;
        other->current = j;
        if (collision_ignores(collision, other)) {
            continue;
        }
        if (collision->shapes[i].flags & SHAPE_OFF ||
            other->shapes[j].flags & SHAPE_OFF) {
            continue;
        }
        switch (other->shapes[j].flags & SHAPE_KIND) {
        case SHAPE_SPHERE:
            collide_sphere_sphere(collision, other);
            break;
        case SHAPE_CYLINDER:
            collide_standing_cylinder(collision, other);
            break;
        case SHAPE_CYLINDER2:
            collide_sphere_cylinder2(collision, other);
            break;
        case SHAPE_CAPSULE:
            collide_sphere_capsule(collision, other);
            break;
        case SHAPE_RECTANGLE:
            collide_sweep_rectangle(collision, other);
            break;
        case SHAPE_RECTANGLE2:
            collide_sweep_rectangle2(collision, other);
            break;
        default:
            break;
        }
    }
    for (i = 1; i < collision->count; i++) {
        for (j = 0; j < other->count; j++) {
            collision->current = i;
            other->current = j;
            if (collision_ignores(collision, other)) {
                continue;
            }
            /* @hack redundant: an else after this if's continue, as in
             * collide_pair (0x8c034bf8); without it, 4 of 312 words
             * differ */
            if (collision->shapes[i].flags & SHAPE_OFF ||
                other->shapes[j].flags & SHAPE_OFF) {
                continue;
            } else if (!(collision->shapes[i].flags & SHAPE_KIND)) {
                switch (other->shapes[j].flags & SHAPE_KIND) {
                case SHAPE_SPHERE:
                    collide_sphere_sphere(collision, other);
                    break;
                case SHAPE_CYLINDER:
                    collide_sphere_cylinder(collision, other);
                    break;
                case SHAPE_CAPSULE:
                    collide_sphere_capsule(collision, other);
                    break;
                case SHAPE_RECTANGLE:
                    collide_sphere_rectangle(collision, other);
                    break;
                }
            } else if ((collision->shapes[i].flags & SHAPE_KIND) ==
                       SHAPE_CYLINDER) {
                switch (other->shapes[j].flags & SHAPE_KIND) {
                case SHAPE_SPHERE:
                    collide_sphere_cylinder(other, collision);
                    break;
                case SHAPE_CYLINDER:
                    collide_cylinder_cylinder(collision, other);
                    break;
                case SHAPE_CAPSULE:
                    collide_cylinder_capsule(collision, other);
                    break;
                case SHAPE_RECTANGLE:
                    break;
                }
            } else if ((collision->shapes[i].flags & SHAPE_KIND) ==
                       SHAPE_CAPSULE) {
                switch (other->shapes[j].flags & SHAPE_KIND) {
                case SHAPE_SPHERE:
                    collide_sphere_capsule(other, collision);
                    break;
                case SHAPE_CYLINDER:
                    collide_cylinder_capsule(other, collision);
                    break;
                case SHAPE_CAPSULE:
                    collide_capsule_capsule(collision, other);
                    break;
                case SHAPE_RECTANGLE:
                    break;
                }
            } else if ((collision->shapes[i].flags & SHAPE_KIND) ==
                       SHAPE_RECTANGLE) {
                switch (other->shapes[j].flags & SHAPE_KIND) {
                case SHAPE_SPHERE:
                    break;
                case SHAPE_CYLINDER:
                    break;
                case SHAPE_RECTANGLE:
                    break;
                /* @hack redundant: a default that only breaks, which has the
                 * loop's test extend j into R3 (EXTU.W R9,R3 at 0x8c034d22);
                 * without it, 2 of 312 words differ */
                default:
                    break;
                }
            }
        }
    }
}
/* NOLINTEND(readability-suspicious-call-argument) */

/* a player's first shape against a wall record's walls and bounds */
static void collide_wall(struct collision_work *collision,
                         struct collision_work *other)
{
    struct collision_shape *shapes = collision->shapes;
    struct collision_shape *walls = other->shapes;
    Uint16 i;
    Uint16 j;

    if (shapes == NULL || walls == NULL) {
        return;
    }
    i = 0;
    for (j = 0; j < other->count; j++) {
        /* @hack redundant: walls, set again and never read, whose read of
         * other gives i R10 and the kind mask R9 (0x8c034d76, 0x8c034d7a);
         * without it, 4 of 77 words differ */
        walls = other->shapes;
        collision->current = i;
        other->current = j;
        if (collision_ignores(collision, other)) {
            continue;
        }
        if (collision->shapes[i].flags & SHAPE_OFF ||
            other->shapes[j].flags & SHAPE_OFF) {
            continue;
        }
        switch (other->shapes[j].flags & SHAPE_KIND) {
        case SHAPE_WALL:
            collide_sweep_wall(collision, other);
            break;
        case SHAPE_BOUND:
            collide_bound(collision, other);
            break;
        }
    }
}

/* an enemy against an object: every shape of each, where a rectangle is
 * landed on rather than swept */
static void collide_enemy_object(struct collision_work *collision,
                                 struct collision_work *other)
{
    struct collision_shape *shapes = collision->shapes;
    struct collision_shape *other_shapes = other->shapes;
    Uint16 i;
    Uint16 j;
    struct task_work *work;

    if (shapes == NULL || other_shapes == NULL) {
        return;
    }
    work = collision->owner->work;
    if (!list_near_and_test_reach(work, collision, other->owner->work, other)) {
        return;
    }
    /* @hack spelling: i = i + 1, which SHC weighs twice and so gives i R11
     * and the hoisted SHAPE_OFF R10 (0x8c034e70, 0x8c034e78); as i++, 11 of
     * 126 words differ */
    for (i = 0; i < collision->count; i = i + 1) {
        for (j = 0; j < other->count; j++) {
            collision->current = i;
            other->current = j;
            if (collision_ignores(collision, other)) {
                continue;
            }
            if (collision->shapes[i].flags & SHAPE_OFF ||
                other->shapes[j].flags & SHAPE_OFF) {
                continue;
            }
            /* @hack redundant: a store of the shape's flags that never runs,
             * one more reference to collision->shapes[i], which puts its offset
             * in R9 and lets SHC AND SHAPE_KIND as an immediate; without it,
             * 110 of 126 words differ */
            if (0) {
                other->current = collision->shapes[i].flags;
            }
            switch (other->shapes[j].flags & SHAPE_KIND) {
            case SHAPE_SPHERE:
                collide_sphere_sphere(collision, other);
                break;
            case SHAPE_CYLINDER:
                collide_standing_cylinder(collision, other);
                break;
            case SHAPE_CAPSULE:
                collide_sphere_capsule(collision, other);
                break;
            case SHAPE_RECTANGLE:
                collide_land_rectangle(collision, other);
                break;
            default:
                break;
            }
        }
    }
}

/* the players against each other and every list, then the attacks
 * against every list but the players', the attacks' and the walls' */
static void collide_players(void)
{
    /* @hack redundant: register on other, which ranks it first: other takes
     * R14, end R13, record R12 and last R11, and the function has no frame;
     * without it, 249 of 252 words differ and collision.c's code runs 32
     * bytes long */
    register struct collision_work **other;
    struct collision_work **end;
    struct collision_work **record;
    struct collision_work **last;

    last = &collision_players[collision_player_count];
    for (record = collision_players; record < last; record++) {
        for (other = record + 1; other < last; other++) {
            collide_pair(*record, *other);
        }
        end = &collision_attacks[collision_attack_count];
        for (other = collision_attacks; other < end; other++) {
            collide_pair(*record, *other);
        }
        end = &collision_items[collision_item_count];
        for (other = collision_items; other < end; other++) {
            collide_target(*record, *other);
        }
        end = &collision_enemies[collision_enemy_count];
        for (other = collision_enemies; other < end; other++) {
            collide_target(*record, *other);
        }
        end = &collision_objects[collision_object_count];
        for (other = collision_objects; other < end; other++) {
            collide_object(*record, *other);
        }
        end = &collision_mutual_objects[collision_mutual_object_count];
        for (other = collision_mutual_objects; other < end; other++) {
            collide_object(*record, *other);
        }
        end = &collision_pickups[collision_pickup_count];
        for (other = collision_pickups; other < end; other++) {
            collide_pair(*record, *other);
        }
        end = &collision_rings[collision_ring_count];
        for (other = collision_rings; other < end; other++) {
            collide_pair(*record, *other);
        }
        /* @hack redundant: a call of njMemCopy that never runs, which gives
         * collision_walls' address R8 (0x8c034fa0); without it,
         * collision_wall_count's address takes R8 and 114 of 252 words
         * differ */
        if (0) {
            njMemCopy((void *)collision_walls, (void *)collision_walls, 0);
        }
        end = &collision_walls[collision_wall_count];
        for (other = collision_walls; other < end; other++) {
            collide_wall(*record, *other);
        }
    }
    last = &collision_attacks[collision_attack_count];
    for (record = collision_attacks; record < last; record++) {
        end = &collision_items[collision_item_count];
        for (other = collision_items; other < end; other++) {
            collide_pair(*record, *other);
        }
        end = &collision_enemies[collision_enemy_count];
        for (other = collision_enemies; other < end; other++) {
            collide_pair(*record, *other);
        }
        end = &collision_objects[collision_object_count];
        for (other = collision_objects; other < end; other++) {
            collide_pair(*record, *other);
        }
        end = &collision_mutual_objects[collision_mutual_object_count];
        for (other = collision_mutual_objects; other < end; other++) {
            collide_pair(*record, *other);
        }
        end = &collision_pickups[collision_pickup_count];
        for (other = collision_pickups; other < end; other++) {
            collide_pair(*record, *other);
        }
        end = &collision_rings[collision_ring_count];
        for (other = collision_rings; other < end; other++) {
            collide_pair(*record, *other);
        }
    }
}

/* the enemies against each other, and against the objects */
static void collide_enemies(void)
{
    Uint16 i;
    Uint16 j;

    for (i = 0; i < collision_enemy_count - 1; i++) {
        for (j = i + 1; j < collision_enemy_count; j++) {
            collide_pair(collision_enemies[i], collision_enemies[j]);
        }
    }
    for (i = 0; i < collision_enemy_count; i++) {
        for (j = 0; j < collision_object_count; j++) {
            collide_enemy_object(collision_enemies[i], collision_objects[j]);
        }
    }
    for (i = 0; i < collision_enemy_count; i++) {
        for (j = 0; j < collision_mutual_object_count; j++) {
            collide_enemy_object(collision_enemies[i],
                                 collision_mutual_objects[j]);
        }
    }
}

/* the mutual objects against each other */
static void collide_mutual(void)
{
    int i;
    int j;

    for (i = 0; i < collision_mutual_object_count - 1; i++) {
        for (j = i + 1; j < collision_mutual_object_count; j++) {
            collide_pair(collision_mutual_objects[i],
                         collision_mutual_objects[j]);
        }
    }
}

void collision_collide_all(void)
{
    if (!edit_mode) {
        near_task_count = 0;
        near_ring_count = 0;
        near_task_count_player1 = 0;
        near_ring_count_player1 = 0;
        near_tasks[0].work = NULL;
        near_rings[0].work = NULL;
        near_tasks_player1[0].work = NULL;
        near_rings_player1[0].work = NULL;
        collide_players();
        collide_enemies();
        collide_mutual();
    } else {
        /* @hack redundant: an empty else, which has the tail call load
         * collision_clear_lists's address into R2, not R3 (0x8c035396);
         * without it, 2 of 30 words differ */
    }
    collision_clear_lists();
}

/* put collision's task just outside the side of other's rectangle that pos,
 * its current shape's center, is nearest. center is the rectangle's center.
 * only x and z move */
static void rectangle_push_out(struct collision_work *collision,
                               NJS_POINT3 *pos, struct collision_work *other,
                               NJS_POINT3 *center)
{
    NJS_POINT3 local;
    struct task_work *work = collision->owner->work;
    struct task_work *other_work = other->owner->work;
    Uint16 index = collision->current;
    Uint16 other_index = other->current;
    struct collision_shape *shape = &collision->shapes[index];
    struct collision_shape *rectangle = &other->shapes[other_index];
    Float radius = shape->radius;
    Float width = rectangle->radius;
    Float depth = rectangle->size_z;
    Float along;
    Float across;
    Float side1;
    Float side2;
    Float dx;
    Float dz;
    Angle ang;

    if ((rectangle->flags & SHAPE_IN_WORLD) ||
        (rectangle->flags & SHAPE_UNTURNED)) {
        ang = -rectangle->ang.y;
    } else {
        ang = -other_work->ang.y - rectangle->ang.y;
    }
    point_to_local(pos, center, ang, &local);
    along = (radius + width) * local.z;
    across = (radius + depth) * local.x;
    side1 = along - across;
    side2 = along + across;
    if (side1 > 0.0F) {
        if (side2 > 0.0F) {
            local.z = (radius + depth) * 0.999F;
        } else {
            local.x = (-radius - width) * 0.999F;
        }
    } else if (side2 > 0.0F) {
        local.x = (radius * 0.999F) + width;
    } else {
        local.z = (-radius - depth) * 0.999F;
    }
    point_from_local(pos, center, ang, &local);
    if (debug_hidden >= 0) {
        njPrintC(NJM_LOCATION(10, 15), "PUSHOUT");
    }
    dx = local.x - pos->x;
    dz = local.z - pos->z;
    work->pos.x = pos->x + dx;
    work->pos.z = pos->z + dz;
}

void collision_free(struct task_work *work)
{
    struct collision_work *collision = work->collision;

    if (collision != NULL) {
        if ((collision->flags & COLLISION_OWN_SHAPES) &&
            collision->shapes != NULL) {
            heap_free(collision->shapes);
            collision->shapes = NULL;
        }
        heap_free(collision);
        work->collision = NULL;
    }
}

Sint8 collision_get_hit_normal(struct task_work *work, NJS_VECTOR *normal)
{
    struct collision_work *collision = work->collision;
    /* @hack spelling: other declared ahead of which, which has SHC load which
     * first (0x8c03559c); declared after it, the loads trade and 4 of 497
     * words differ */
    struct collision_work *other = collision->hit;
    Sint16 which = collision->hit_shape;
    struct collision_shape *shape = collision->shapes;
    struct collision_shape *other_shape;
    struct task_work *other_work;
    Float radius;
    Float width;
    Float depth;
    Float along;
    Float across;
    Float side1;
    Float side2;
    Angle ang;
    Angle face;
    NJS_POINT3 pos;
    NJS_POINT3 other_pos;
    NJS_POINT3 local;

    /* @hack redundant: the count tested in an else, which puts which's EXTS.W
     * in R3 (0x8c0355ae); in an if of its own, R2, and 2 of 497 words differ */
    if (other == NULL) {
        return 0;
    } else if (which > other->count) {
        return 0;
    }
    other_shape = &other->shapes[which];
    if (shape == NULL) {
        return 0;
    }
    if (other_shape == NULL) {
        return 0;
    }
    radius = shape->radius;
    width = other_shape->radius;
    depth = other_shape->size_z;
    other_work = other->owner->work;
    shape_get_center(work, shape, &pos);
    switch (other_shape->flags & SHAPE_KIND) {
    case SHAPE_SPHERE:
        shape_get_center(other_work, other_shape, &other_pos);
        normal->x = pos.x - other_pos.x;
        normal->y = pos.y - other_pos.y;
        normal->z = pos.z - other_pos.z;
        if ((normal->x * normal->x) + (normal->y * normal->y) +
                (normal->z * normal->z) !=
            0.0F) {
            njUnitVector(normal);
            normal->x *= -1.0F;
            normal->y *= -1.0F;
            normal->z *= -1.0F;
        } else {
            normal->x = 0.0F;
            normal->y = 0.0F;
            normal->z = 1.0F;
        }
        break;
    case SHAPE_CYLINDER:
        shape_get_center(other_work, other_shape, &other_pos);
        normal->x = pos.x - other_pos.x;
        normal->y = 0.0F;
        normal->z = pos.z - other_pos.z;
        if ((normal->x * normal->x) + (normal->y * normal->y) +
                (normal->z * normal->z) !=
            0.0F) {
            njUnitVector(normal);
        } else {
            normal->x = 0.0F;
            normal->y = 0.0F;
            normal->z = 1.0F;
        }
        break;
    case SHAPE_CYLINDER2:
        collision_get_shape_center(other_work, other_shape, &other_pos);
        normal->x = pos.x - other_pos.x;
        normal->y = 0.0F;
        normal->z = pos.z - other_pos.z;
        if ((normal->x * normal->x) + (normal->y * normal->y) +
                (normal->z * normal->z) !=
            0.0F) {
            njUnitVector(normal);
        } else {
            normal->x = 0.0F;
            normal->y = 0.0F;
            normal->z = 1.0F;
        }
        break;
    case SHAPE_CAPSULE:
        collision_get_shape_center(other_work, other_shape, &other_pos);
        normal->x = pos.x - other_pos.x;
        normal->y = 0.0F;
        normal->z = pos.z - other_pos.z;
        if ((normal->x * normal->x) + (normal->y * normal->y) +
                (normal->z * normal->z) !=
            0.0F) {
            njUnitVector(normal);
        } else {
            normal->x = 0.0F;
            normal->y = 0.0F;
            normal->z = 1.0F;
        }
        break;
    case SHAPE_RECTANGLE:
        pos.x = work->pos.x;
        pos.y = work->pos.y;
        pos.z = work->pos.z;
        shape_get_box_center(other_work, other_shape, &other_pos);
        if ((other_shape->flags & SHAPE_IN_WORLD) ||
            (other_shape->flags & SHAPE_UNTURNED)) {
            ang = -other_shape->ang.y;
        } else {
            ang = -other_work->ang.y - other_shape->ang.y;
        }
        point_to_local(&pos, &other_pos, ang, &local);
        along = (radius + width) * local.z;
        across = (radius + depth) * local.x;
        side1 = along - across;
        side2 = along + across;
        if (side1 > 0.0F) {
            if (side2 > 0.0F) {
                face = 0;
                if (debug_hidden >= 0) {
                    njPrintC(NJM_LOCATION(50, 50), "side1");
                }
            } else {
                face = NJM_DEG_ANG(270);
                if (debug_hidden >= 0) {
                    njPrintC(NJM_LOCATION(50, 50), "side2");
                }
            }
        } else if (side2 > 0.0F) {
            face = NJM_DEG_ANG(90);
            if (debug_hidden >= 0) {
                njPrintC(NJM_LOCATION(50, 50), "side3");
            }
        } else {
            face = NJM_DEG_ANG(180);
            if (debug_hidden >= 0) {
                njPrintC(NJM_LOCATION(50, 50), "side4");
            }
        }
        local.x = 0.0F;
        local.y = 0.0F;
        local.z = 1.0F;
        njPushMatrix(&_unit_matrix_);
        TURN(njRotateY, face - ang);
        njCalcVector(NULL, &local, normal);
        njPopMatrix(1);
        break;
    case SHAPE_RECTANGLE2:
        shape_get_center(other_work, other_shape, &other_pos);
        normal->x = pos.x - other_pos.x;
        normal->y = 0.0F;
        normal->z = pos.z - other_pos.z;
        if ((normal->x * normal->x) + (normal->y * normal->y) +
                (normal->z * normal->z) !=
            0.0F) {
            njUnitVector(normal);
        } else {
            normal->x = 0.0F;
            normal->y = 0.0F;
            normal->z = 1.0F;
        }
        break;
    default:
        return 0;
    }
    if (debug_hidden >= 0) {
        draw_line(other_pos.x, other_pos.y, other_pos.z,
                  other_pos.x + (normal->x * 15.0F), other_pos.y,
                  other_pos.z + (normal->z * 15.0F));
    }
    return 1;
}
