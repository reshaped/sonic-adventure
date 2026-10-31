/* helpers: small routines the objects and the race share, from vector sums
 * to mirrors and jumps */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "collision/world.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "object/common.h"
#include "object/helpers.h"
#include "object/item_box.h"
#include "object/placement.h"
#include "object/ring.h"
#include "player/player.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* bits of work->tick, for a task that keeps no count there */
#define TICK_TOUCHED 0x1 /* object_touch_moved holds a touch */
#define TICK_NO_WORLD 0x2 /* object_add_world_model found the pool empty */

/* a player within a world model's radius plus this of its task has the
 * world test the model */
#define WORLD_REACH 20.0F

/* a jump that ends within this of its target lands on it */
#define LANDED_RANGE 2.0F

/* an arrow one unit long along y, which arrow_draw_y stretches */
static NJS_MATERIAL arrow_materials[1] = {
#include "object/helpers/arrow_materials.inc"
};
static Sint16 arrow_meshes[102] = {
#include "object/helpers/arrow_meshes.inc"
};
static NJS_MESHSET arrow_meshsets[1] = {
#include "object/helpers/arrow_meshsets.inc"
};
static NJS_POINT3 arrow_points[19] = {
#include "object/helpers/arrow_points.inc"
};
static NJS_VECTOR arrow_normals[19] = {
#include "object/helpers/arrow_normals.inc"
};
static NJS_MODEL arrow_model = {
#include "object/helpers/arrow_model.inc"
};
NJS_OBJECT arrow_y = {
#include "object/helpers/arrow_y.inc"
};
/* the same arrow turned to lie along z, which arrow_draw stretches */
static NJS_OBJECT arrow_z = {
#include "object/helpers/arrow_z.inc"
};

/* quad_order[n] is the corner a quad's nth point takes in its
 * mirror_quad. a model lists a quad's points in strip order, and the
 * corners run round its edge */
static Sint32 quad_order[4] = {0, 1, 3, 2};

void vector_copy(const Float *from, Float *to)
{
    /* @hack spelling: from and to walked, for the image's @Rn+ loads and ADD #4
     * stores (0x8c069aa0, 0x8c069aa4); without it, through subscripts, 8 of
     * 9 words differ */
    *to++ = *from++;
    *to++ = *from++;
    *to = *from;
}

void angles_copy(const Angle *from, Angle *to)
{
    /* @hack spelling: from and to walked, as in vector_copy (0x8c069ab2,
     * 0x8c069ab6); without it, through subscripts, 9 of 9 words differ */
    *to++ = *from++;
    *to++ = *from++;
    *to = *from;
}

void vector_clear(Float *vector)
{
    /* @hack spelling: vector walked, for the image's ADD #4 stores
     * (0x8c069ac8); without it, through subscripts, 6 of 7 words differ */
    *vector++ = 0.0F;
    *vector++ = 0.0F;
    *vector = 0.0F;
}

void vector_add(const NJS_VECTOR *first, const NJS_VECTOR *second,
                NJS_VECTOR *sum)
{
    /* @hack spelling: byte cursors over the whole vectors, for the image's
     * @Rn+ loads and ADD #4 stores; without them, through the members,
     * 12 of 15 words differ */
    const char *at_first = (const char *)first;
    const char *at_second = (const char *)second;
    char *out = (char *)sum;

    *(Float *)out = *(const Float *)at_first + *(const Float *)at_second;
    out += sizeof(Float);
    at_first += sizeof(Float);
    at_second += sizeof(Float);
    *(Float *)out = *(const Float *)at_first + *(const Float *)at_second;
    out += sizeof(Float);
    at_first += sizeof(Float);
    at_second += sizeof(Float);
    *(Float *)out = *(const Float *)at_first + *(const Float *)at_second;
}

void vector_sub(const Float *first, const Float *second, Float *difference)
{
    /* @hack spelling: the three pointers walked, as in vector_copy; without it,
     * through subscripts, 12 of 15 words differ */
    *difference++ = *first++ - *second++;
    *difference++ = *first++ - *second++;
    *difference = *first - *second;
}

void vector_between(const NJS_VECTOR *from, const NJS_VECTOR *to,
                    NJS_VECTOR *out)
{
    /* @hack spelling: byte cursors over the whole vectors, as in vector_add;
     * without them, through the members, 12 of 15 words differ */
    const char *at_from = (const char *)from;
    const char *at_to = (const char *)to;
    char *at_out = (char *)out;

    *(Float *)at_out = *(const Float *)at_to - *(const Float *)at_from;
    at_out += sizeof(Float);
    at_to += sizeof(Float);
    at_from += sizeof(Float);
    *(Float *)at_out = *(const Float *)at_to - *(const Float *)at_from;
    at_out += sizeof(Float);
    at_to += sizeof(Float);
    at_from += sizeof(Float);
    *(Float *)at_out = *(const Float *)at_to - *(const Float *)at_from;
}

void object_step_draw_range(struct task *task)
{
    /* @hack spelling: the draw called through a local, which SHC stores to the
     * stack (MOV.L R3,@R15 at 0x8c069b38); called as task->draw, 13 of 15
     * words differ */
    void (*draw)(struct task *task);

    draw = task->draw;
    draw(task);
    placement_try_leave(task);
}

void object_step_draw(struct task *task)
{
    /* @hack spelling: the draw called through a local, as above (MOV.L R3,@R15
     * at 0x8c069b50); called as task->draw, 6 of 6 words differ */
    void (*draw)(struct task *task);

    draw = task->draw;
    draw(task);
}

void object_draw_model(struct task *task)
{
    struct task_work *work = task->work;

    njSetTexture(MODEL(work)->texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    draw_tree(MODEL(work)->object);
    njPopMatrix(1);
}

void object_draw_model_scaled(struct task *task)
{
    struct task_work *work = task->work;

    njSetTexture(MODEL(work)->texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    njScaleV(NULL, &work->scl);
    draw_tree(MODEL(work)->object);
    njPopMatrix(1);
}

void object_draw_action(struct task *task)
{
    struct task_work *work = task->work;

    njSetTexture(ACTION(work)->texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    /* @hack spelling: the frame added in the test, which compares the sum in
     * FR2 (FCMP/GT FR2,FR1 at 0x8c069cd6); added apart, SHC loads the frame
     * back from the work and 9 of 70 words differ */
    if ((ACTION(work)->frame += 1.0F) >=
        (Float)ACTION(work)->action->motion->nbFrame) {
        ACTION(work)->frame = 0.0F;
    }
    draw_action(ACTION(work)->action, ACTION(work)->frame);
    njPopMatrix(1);
}

int object_touch_moved(struct task_work *work, struct touch *touch)
{
    struct collision_work *collision;
    struct collision_work *hit;
    struct collision_shape *shape;
    Float *moved;

    if (work->flags & COLLISION_DAMAGED) {
        collision = work->collision;
        hit = collision->hit;
        shape = &hit->shapes[collision->hit_shape];
        if (!(work->tick & TICK_TOUCHED)) {
            work->tick |= TICK_TOUCHED;
            touch->moved.x = shape->center.x;
            touch->moved.y = shape->center.y;
            touch->moved.z = shape->center.z;
            touch->by.shape = shape;
            return 0;
        } else if (shape == touch->by.shape) {
            goto moved;
        }
    }
    work->tick &= ~TICK_TOUCHED;
    return 0;
    /* @hack spelling: the report's arm placed last, past the shared return of
     * 0 (0x8c069d44); in the else-if's arm, 36 of 69 words differ */
moved:
    /* @hack spelling: each of touch->moved's members through a pointer, whose
     * address SHC forms in R4 (MOV R5,R4 at 0x8c069d46); through
     * touch->moved, SHC reaches y and z through R0 and 14 of 69 words
     * differ */
    moved = &touch->moved.x;
    *moved = shape->center.x - *moved;
    moved = &touch->moved.y;
    *moved = shape->center.y - *moved;
    moved = &touch->moved.z;
    *moved = shape->center.z - *moved;
    touch->by.owner = hit->owner;
    return 1 << PLAYER_VARS(hit->owner->work)->character;
}

Float vector_dot(NJS_VECTOR *first, NJS_VECTOR *second)
{
    return (first->x * second->x) + (first->y * second->y) +
           (first->z * second->z);
}

void vector_cross(NJS_VECTOR *first, NJS_VECTOR *second, NJS_VECTOR *out)
{
    out->x = (first->y * second->z) - (first->z * second->y);
    out->y = (first->z * second->x) - (first->x * second->z);
    out->z = (first->x * second->y) - (first->y * second->x);
}

Float vector_length(NJS_VECTOR *vector)
{
    return njSqrt((vector->x * vector->x) + (vector->y * vector->y) +
                  (vector->z * vector->z));
}

void vector_aim_z(NJS_VECTOR *vector, Angle *ang_x, Angle *ang_y)
{
    *ang_x = -(Angle)(atan2f(vector->y, vector->z) * ANGLE_PER_RADIAN);
    *ang_y = (Angle)(atan2f(vector->x, njSqrt((vector->y * vector->y) +
                                              (vector->z * vector->z))) *
                     ANGLE_PER_RADIAN);
}

void vector_aim_y(NJS_VECTOR *vector, Angle *ang_x, Angle *ang_z)
{
    *ang_z = -(Angle)(atan2f(vector->x, vector->y) * ANGLE_PER_RADIAN);
    *ang_x = (Angle)(atan2f(vector->z, njSqrt((vector->x * vector->x) +
                                              (vector->y * vector->y))) *
                     ANGLE_PER_RADIAN);
}

void vector_aim_x(NJS_VECTOR *vector, Angle *ang_y, Angle *ang_z)
{
    *ang_y = -(Angle)(atan2f(vector->z, vector->x) * ANGLE_PER_RADIAN);
    *ang_z = (Angle)(atan2f(vector->y, njSqrt((vector->z * vector->z) +
                                              (vector->x * vector->x))) *
                     ANGLE_PER_RADIAN);
}

Float vector_reflect(NJS_VECTOR *vector, NJS_VECTOR *normal, NJS_VECTOR *out)
{
    Float dot = vector_dot(vector, normal);
    Float push = -(dot * 2.0F);
    Float squareness = dot / vector_length(vector);

    out->x = vector->x + (push * normal->x);
    out->y = vector->y + (push * normal->y);
    out->z = vector->z + (push * normal->z);
    return njAbs(squareness);
}

void arrow_draw(NJS_POINT3 *pos, NJS_VECTOR *vector)
{
    /* @hack type: the angles in an array, whose elements' addresses SHC
     * keeps on the stack (MOV.L R6,@(12,R15) at 0x8c06a06a); as two Angle
     * locals, 52 of 84 words differ */
    Angle aim[3];
    Float length;

    if (frame_step != 0) {
        length = njSqrt((vector->x * vector->x) + (vector->y * vector->y) +
                        (vector->z * vector->z));
        vector_aim_z(vector, &aim[0], &aim[1]);
        njPushMatrix(NULL);
        njTranslateV(NULL, pos);
        TURN(njRotateX, aim[0]);
        TURN(njRotateY, aim[1]);
        njScale(NULL, 1.0F, 1.0F, length);
        njSimpleDrawObject(&arrow_z);
        njPopMatrix(1);
    }
}

void arrow_draw_y(NJS_POINT3 *pos, NJS_VECTOR *vector)
{
    /* @hack type: the angles in an array, as in arrow_draw; as two Angle
     * locals, 52 of 84 words differ */
    Angle aim[3];
    Float length;

    if (frame_step != 0) {
        length = njSqrt((vector->x * vector->x) + (vector->y * vector->y) +
                        (vector->z * vector->z));
        vector_aim_y(vector, &aim[0], &aim[2]);
        njPushMatrix(NULL);
        njTranslateV(NULL, pos);
        TURN(njRotateZ, aim[2]);
        TURN(njRotateX, aim[0]);
        njScale(NULL, 1.0F, length, 1.0F);
        njSimpleDrawObject(&arrow_y);
        njPopMatrix(1);
    }
}

void marker_draw(NJS_POINT3 *pos, Float size)
{
    if (frame_step != 0) {
        size *= 0.1F;
        njPushMatrix(NULL);
        njTranslateV(NULL, pos);
        njScale(NULL, size, size, size);
        njSimpleDrawObject(&marker_object);
        njPopMatrix(1);
    }
}

int item_pick(Float *table)
{
    Float pick = (Float)rand() * RAND_SCALE;
    Float weight;

    while ((weight = *table++) != 0.0F) {
        if ((pick -= weight) < 0.0F) {
        /* @hack redundant: a label no goto uses, which has SHC load the kind
         * into FR3 (FMOV.S @R14,FR3 at 0x8c06a228); without it, FR2, and 2 of
         * 31 words differ */
        found: /* NOLINT(clang-diagnostic-unused-label) */
            return (int)*table;
        }
        table++;
    }
    return 0;
}

Bool item_drop(struct task_work *work, enum drop_kind kind)
{
    switch (kind) {
    case DROP_RING:
        ring_scatter(work->pos.x, work->pos.y, work->pos.z, 1);
        break;
    case DROP_5_RINGS:
        ring_scatter(work->pos.x, work->pos.y, work->pos.z, 5);
        break;
    case DROP_10_RINGS:
        ring_scatter(work->pos.x, work->pos.y, work->pos.z, 10);
        break;
    case DROP_RANDOM_RINGS:
        item_box_create(&work->pos, ITEM_RANDOMRING);
        break;
    case DROP_1UP:
        item_box_create(&work->pos, ITEM_1UP);
        break;
    }
    return TRUE;
}

/* point lies inside while, seen from it, each corner turns anticlockwise
 * about normal to the next. offsets[n] is corner n less point */
Bool polygon_contains(NJS_POINT3 *point, NJS_POINT3 *corners,
                      NJS_VECTOR *normal, int count)
{
    NJS_VECTOR offsets[8];
    NJS_VECTOR cross;
    int i;

    for (i = 0; i < count; i++) {
        offsets[i].x = corners->x - point->x;
        offsets[i].y = corners->y - point->y;
        offsets[i].z = corners->z - point->z;
        corners++;
    }
    /* the first offset again after the last, for the last turn */
    /* @hack type: the offset's Floats moved as three Sint32 words through
     * words_copy, which SHC expands to MOV.L moves; without it, member by
     * member, 54 of 96 words differ */
    words_copy((Sint32 *)offsets, (Sint32 *)&offsets[i], 3);
    for (i = 0; i < count; i++) {
        /* @hack type: the next offset's index narrowed to a Sint16, which SHC
         * scales apart from i's (ADD #1,R5 at 0x8c06a338); as i + 1, SHC
         * adds 12 to &offsets[i] in a fourth saved register, R11, and 85 of
         * 96 words differ */
        vector_cross(&offsets[i], &offsets[(Sint16)(i + 1)], &cross);
        if (vector_dot(&cross, normal) <= 0.000001F) {
            return FALSE;
        }
    }
    return TRUE;
}

/* polygon_contains, the only code that calls words_copy, expands it in place
 * (0x8c06a320). words_copy, being external, keeps its own body too, at
 * 0x8c06a380. SHC writes no copy of a static it expands at every call */
void words_copy(Sint32 *from, Sint32 *to, int count)
{
    while (count--) {
        *to++ = *from++;
    }
}

Bool player_get_middle(Uint8 player, int steps, NJS_POINT3 *pos,
                       struct angles *ang)
{
    struct player *record;
    NJS_VECTOR up;

    if (!player_get_history(player, steps, pos, ang)) {
        return FALSE;
    }
    record = player_get(player);
    up.x = 0.0F;
    up.y = record->consts.height * 0.5F;
    up.z = 0.0F;
    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    njTranslateV(NULL, pos);
    TURN(njRotateZ, ang->z);
    TURN(njRotateX, ang->x);
    TURN(njRotateY, ang->y);
    njCalcPoint(NULL, &up, pos);
    njPopMatrix(1);
    return TRUE;
}

void mirror_clear(void)
{
    mirrors.count = 0;
}

int mirror_count(void)
{
    return mirrors.count;
}

Bool mirror_add(struct task_work *work, NJS_MODEL *model)
{
    NJS_MESHSET *meshset = model->meshsets;
    int slot;
    struct mirror *mirror;
    struct mirror_quad *quad;
    Sint16 *meshes;
    NJS_POINT3 *points;
    NJS_VECTOR *normal;
    Sint32 *order;
    Sint32 *corner;
    int count;
    int i;
    int j;
    NJS_POINT3 *point;

    if ((slot = mirrors.count++) >= MIRRORS) {
        return FALSE;
    }
    /* @hack spelling: an empty arm for a meshset of other polygons, which puts
     * the mirror's size in R3 and njPushMatrix's address in R1 (0x8c06a4fc,
     * 0x8c06a506); as the quads' test alone, 12 of 153 words differ */
    if ((meshset->type_matId & NJD_MESHSET_MASK) != NJD_MESHSET_4) {
    } else {
        meshes = meshset->meshes;
        /* @hack type: mirror set to the list's first ahead of points, then
         * moved on by an offset narrowed to a Sint16, the image's MULS.W and
         * EXTS.W (0x8c06a502, 0x8c06a510). the test above keeps slot below
         * MIRRORS, where the offset fits a Sint16 and the narrowing changes
         * nothing; as &mirrors.mirrors[slot] after points, 9 of 153 words
         * differ */
        mirror = mirrors.mirrors;
        points = model->points;
        mirror = (struct mirror *)((char *)mirror +
                                   (Sint16)(slot * sizeof(*mirror)));
        quad = mirror->quads;
        mirror->count = count = meshset->nbMesh;
        njPushMatrix(&_unit_matrix_);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateZ, work->ang.z);
        TURN(njRotateX, work->ang.x);
        TURN(njRotateY, work->ang.y);
        /* @hack spelling: i zeroed ahead of normal and order, which puts its 0
         * in R3 (MOV #0,R3 at 0x8c06a558); zeroed in the for, 6 of 153 words
         * differ */
        i = 0;
        normal = meshset->normals;
        order = quad_order;
        for (; i < count; i++) {
            /* @hack spelling: the vectors through from and to, which put MOV
             * R13,R6 ahead of MOV R8,R5 (0x8c06a56c); passed directly, 2 of
             * 153 words differ */
            NJS_VECTOR *from = normal;
            NJS_VECTOR *to = &quad->normal;

            njCalcVector(NULL, from, to);
            corner = order;
            for (j = 0; j < 4; j++) {
                /* @hack spelling: the point through a local, which keeps
                 * meshes in R9 and njCalcPoint's address in R10 (0x8c06a578,
                 * 0x8c06a596); passed directly, 33 of 153 words differ */
                point = &points[*meshes++];
                njCalcPoint(NULL, point, &quad->corners[*corner]);
                corner++;
            }
            quad++;
            /* @hack spelling: normal = normal + 1, which keeps the stack frame
             * at 44 bytes (ADD #-12,R15 at 0x8c06a4d0); as normal++, 48 bytes,
             * and 99 of 153 words differ */
            normal = normal + 1;
        }
        njPopMatrix(1);
        return TRUE;
    }
    return FALSE;
}

void mirror_draw(struct task *task, void (*draw)(struct task *task))
{
    int count;
    struct task_work *work = task->work;
    struct mirror *mirror;
    struct mirror_quad *quad;
    NJS_POINT3 nearest; /* the point of the quad's plane nearest the task */
    NJS_VECTOR delta;
    NJS_PLANE plane;
    int quads;
    int i;
    int j;

    /* @hack redundant: count tested ahead of the loop's own test, the image's
     * TST R3,R3 ahead of its CMP/PL R3 (0x8c06a656, 0x8c06a664); without
     * it, 124 of 135 words differ */
    if ((count = mirrors.count) != 0) {
        /* @bug memory: count is mirror_add's calls since mirror_clear,
         * which can pass MIRRORS, and the loop then reads past
         * mirrors.mirrors (CMP/PL R3 at 0x8c06a664 bounds it by count
         * alone) */
        for (i = 0; i < count; i++) {
            mirror = &mirrors.mirrors[i];
            quads = mirror->count;
            quad = mirror->quads;
            for (j = 0; j < quads; j++) {
                vector_copy(&quad->corners[0].x, &plane.px);
                vector_copy(&quad->normal.x, &plane.vx);
                njDistanceP2PL(&work->pos, &plane, &nearest);
                delta.x = nearest.x - work->pos.x;
                delta.y = nearest.y - work->pos.y;
                delta.z = nearest.z - work->pos.z;
                if (vector_dot(&delta, &quad->normal) < 0.0F &&
                    polygon_contains(&nearest, quad->corners, &quad->normal,
                                     4)) {
                    marker_draw(&nearest, 0.5F);
                    njPushMatrix(NULL);
                    njMirror(NULL, &plane);
                    draw(task);
                    marker_draw(&work->pos, 1.0F);
                    njPopMatrix(1);
                }
                quad++;
            }
        }
    }
}

NJS_OBJECT *object_add_world_model(struct task *task, NJS_MODEL *model)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;

    if ((object = world_object_alloc()) != NULL) {
        object->evalflags = NJD_EVAL_BREAK;
        object->scl[0] = 1.0F;
        object->scl[1] = 1.0F;
        object->scl[2] = 1.0F;
        object->pos[0] = work->pos.x;
        object->pos[1] = work->pos.y;
        object->pos[2] = work->pos.z;
        object->ang[0] = work->ang.x;
        object->ang[1] = work->ang.y;
        object->ang[2] = work->ang.z;
        object->model = model;
        object->child = NULL;
        object->sibling = NULL;
        world_add(0, task, object);
        return object;
    }
    work->tick |= TICK_NO_WORLD;
    return NULL;
}

void object_world_follow(struct task_work *work, NJS_OBJECT *object)
{
    if (!(work->tick & TICK_NO_WORLD)) {
        if (player_in_range(&work->pos, object->model->r + WORLD_REACH)) {
            work->flags |= WORLD_TESTED;
            object->pos[0] = work->pos.x;
            object->pos[1] = work->pos.y;
            object->pos[2] = work->pos.z;
            object->ang[0] = work->ang.x;
            object->ang[1] = work->ang.y;
            object->ang[2] = work->ang.z;
        } else {
            work->flags &= ~WORLD_TESTED;
        }
    }
}

void object_world_test(struct task_work *work, NJS_OBJECT *object)
{
    if (!(work->tick & TICK_NO_WORLD)) {
        if (player_in_range(&work->pos, object->model->r + WORLD_REACH)) {
            work->flags |= WORLD_TESTED;
        } else {
            work->flags &= ~WORLD_TESTED;
        }
    }
}

void collision_shapes_on(struct task_work *work)
{
    /* @hack spelling: the count through a local, which SHC stores to the
     * stack and loads back (MOV.W R0,@R15 and MOV.W @R15,R6 at 0x8c06a8dc);
     * read in the test, 20 of 20 words differ */
    Uint16 count = work->collision->count;
    struct collision_shape *shape = work->collision->shapes;
    int i;

    for (i = 0; i < count; i++) {
        shape->flags &= ~SHAPE_OFF;
        shape++;
    }
}

void collision_shapes_off(struct task_work *work)
{
    /* @hack spelling: the count through a local, as above (0x8c06a928); read
     * in the test, 20 of 20 words differ */
    Uint16 count = work->collision->count;
    struct collision_shape *shape = work->collision->shapes;
    int i;

    for (i = 0; i < count; i++) {
        shape->flags |= SHAPE_OFF;
        shape++;
    }
}

void action_advance(NJS_ACTION *action, Float *frame, Float step)
{
    *frame += step;
    if (*frame < 0.0F) {
        *frame += action->motion->nbFrame;
        return;
    }
    if (*frame >= action->motion->nbFrame) {
        *frame -= action->motion->nbFrame;
    }
}

int players_near(NJS_POINT3 *pos, Float range)
{
    int near = 0;
    int i;
    struct task *task;
    const char *center;
    Float distance;

    for (i = 0; i < 2; i++) {
        /* @hack spelling: task assigned in the test, which tests R0 with the
         * copy to R4 in the BT/S's slot (0x8c06a9d4, 0x8c06a9d8); assigned
         * apart, 3 of 66 words differ */
        if ((task = player_get_task(i)) == NULL) {
            continue;
        }
        /* @hack spelling: a byte cursor over the shape's center, for the
         * image's
         * @Rn+ loads; without it, through a pointer to the center, 3 of 66
         * words differ */
        center = (const char *)&task->work->collision->shapes->center;
        scratch_vector.x = *(const Float *)center - pos->x;
        center += sizeof(Float);
        scratch_vector.y = *(const Float *)center - pos->y;
        center += sizeof(Float);
        scratch_vector.z = *(const Float *)center - pos->z;
        /* @hack spelling: the distance through a local, which SHC copies
         * from FR0 to FR4 for the compare (FMOV FR0,FR4 at 0x8c06aa04);
         * compared directly, 22 of 66 words differ */
        distance = njScalor2(&scratch_vector);
        if (distance < range) {
            near |= 1 << i;
        }
    }
    return near;
}

Float vector_distance2(NJS_POINT3 *from, NJS_POINT3 *to)
{
    NJS_VECTOR delta;

    delta.x = from->x - to->x;
    delta.y = from->y - to->y;
    delta.z = from->z - to->z;
    return njScalor2(&delta);
}

void jump_aim(struct jump *jump)
{
    Float time = (Float)jump->steps;
    /* @hack spelling: pos through a pointer to the point, which puts dx in FR14
     * and dz in FR13; as &jump->pos.x, they trade and 10 of 80 words
     * differ */
    NJS_POINT3 *from = &jump->pos;
    /* @hack spelling: a Float cursor stepped across pos's members, for the
     * image's @Rn+ loads; as from->x, ->y and ->z, the function runs 4 bytes
     * long and 66 of 80 words differ */
    Float *pos = &from->x;
    /* @hack spelling: a Float cursor stepped across target's members the same
     * way; as jump->target.x, .y and .z, the function runs 2 bytes long and 70
     * of 80 words differ */
    Float *target = &jump->target.x;
    Float dx;
    Float dy;
    Float dz;
    Float level;
    Float speed;
    Angle ang;

    dx = *target++ - *pos++;
    dy = *target++ - *pos++;
    dz = *target - *pos;
    level = njSqrt((dx * dx) + (dz * dz));
    speed = level / time;
    ang = (Angle)(atan2f(dz, dx) * ANGLE_PER_RADIAN);
    jump->speed.x = speed * njCos(ang);
    jump->speed.z = speed * njSin(ang);
    jump->speed.y =
        ((dy * 2.0F) + (jump->gravity * time * time)) / (time * 2.0F);
}

enum jump_state jump_landed(struct jump *jump)
{
    /* @hack spelling: byte cursors over pos and target, for the image's @Rn+
     * loads; without them, through pointers to the points, 6 of 42 words
     * differ */
    const char *pos = (const char *)&jump->pos;
    const char *target = (const char *)&jump->target;
    Float distance;

    if (jump->steps-- > 0) {
        return JUMP_FLYING;
    }
    scratch_vector.x = *(const Float *)target - *(const Float *)pos;
    target += sizeof(Float);
    pos += sizeof(Float);
    scratch_vector.y = *(const Float *)target - *(const Float *)pos;
    target += sizeof(Float);
    pos += sizeof(Float);
    scratch_vector.z = *(const Float *)target - *(const Float *)pos;
    distance = njAbs(njScalor(&scratch_vector));
    if (distance < LANDED_RANGE) {
        return JUMP_ON_TARGET;
    }
    return JUMP_OFF_TARGET;
}

Float vector_cos(NJS_VECTOR *first, NJS_VECTOR *second)
{
    vector_normalize(first);
    vector_normalize(second);
    return njInnerProduct(first, second);
}

Bool stage_before_bosses(void)
{
    Uint16 stage = (Uint8)(stage_get_stage_act() >> 8);

    if (stage >= STAGE_PRACTICE && stage < STAGE_CHAOS0) {
        return TRUE;
    }
    return FALSE;
}

Float vector_normalize(NJS_VECTOR *vector)
{
    NJS_VECTOR copy;
    Float length;

    vector_copy(&vector->x, &copy.x);
    length = njScalor(&copy);
    if (length < 0.000001F) {
        vector->x = 0.0F;
        vector->y = 1.0F;
        vector->z = 0.0F;
    }
    njUnitVector(vector);
    return length;
}

void object_idle(struct task *task)
{
}
