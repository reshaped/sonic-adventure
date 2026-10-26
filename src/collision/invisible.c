/* stage objects whose shapes collide but draw only in the set editor */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "collision/ground.h"
#include "collision/invisible.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "object/placement.h"
#include "player/player.h"
#include "system/task.h"
#include "system/util.h"
#include "system/vector.h"

/* the shapes invisible_draw draws, numbered as SHAPE_*'s kinds but for
 * the wall */
#define DRAW_SPHERE 0
#define DRAW_CYLINDER 1
#define DRAW_BOX 4
#define DRAW_BOUND 7
#define DRAW_WALL 8

/* the object's modes, in its work's mode: the first step sets the object
 * up, and the steps after it run it */
enum invisible_mode { INVISIBLE_START, INVISIBLE_RUN };

/* a shape's radius and half sizes are its task's scale plus this */
#define MARGIN 10.0F

/* a bound is destroyed once neither the camera nor player 0 is within 3000
 * of it. placement_try_leave_by takes the range squared */
#define BOUND_RANGE (3000.0F * 3000.0F)

/* a zero word of unknown type, which no code names */
static Uint32 dat_8c1aba18 = 0;

/* a solid shape pushes every group of kinds, and no group can push it */
#define PUSH_SOLID (PUSH_ALL | PUSH_FIRM(PUSH_ALL))

/* each object's shape. collision_init copies it, and invisible_set_size
 * sizes the copy to the task's scale. all are solid but the ottotto box's,
 * which pushes nothing. the jump box shares it */
/* clang-format off */
static struct collision_shape invisible_sphere_shape = {
    SHAPE_SPHERE, 0, PUSH_SOLID,
    {0.0F, 0.0F, 0.0F}, 10.0F, 10.0F, 10.0F, {0}, 0, NULL};
static struct collision_shape invisible_cylinder_shape = {
    SHAPE_CYLINDER, 0, PUSH_SOLID,
    {0.0F, 0.0F, 0.0F}, 10.0F, 10.0F, 10.0F, {0}, 0, NULL};
static struct collision_shape invisible_box_shape = {
    SHAPE_RECTANGLE | SHAPE_NO_PUSH, 0, PUSH_SOLID,
    {0.0F, 0.0F, 0.0F}, 40.0F, 30.0F, 10.0F, {0}, 0, NULL};
static struct collision_shape invisible_wall_shape = {
    SHAPE_WALL, 0, PUSH_SOLID,
    {0.0F, 0.0F, 0.0F}, 40.0F, 30.0F, 10.0F, {0}, 0, NULL};
static struct collision_shape ottotto_shape = {
    SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F}, 40.0F, 30.0F, 10.0F, {0}, 0, NULL};
static struct collision_shape invisible_bound_shape = {
    SHAPE_BOUND, 0, PUSH_SOLID,
    {0.0F, 0.0F, 0.0F}, 60.0F, 1.0F, 10.0F, {0}, 0, NULL};
/* a sphere of radius 10 at the camera, which the dry box tests */
/* @hack type: an array of one, whose address the image loads once to store
 * the center through and to pass (MOV.L R3,@(8,R1) at 0x8c02f15a); declared
 * as one shape, SHC loads the center's own address too and 18 of
 * invisible_dry_step's 153 words differ */
static struct collision_shape invisible_camera_shape[1] = {
    {SHAPE_SPHERE, 0, PUSH_SOLID,
     {0.0F, 0.0F, 0.0F}, 10.0F, 10.0F, 10.0F, {0}, 0, NULL},
};
/* clang-format on */

/* draw the object's shape of kind at its position, turned by its y angle
 * and sized as invisible_set_size sizes the shape. a wall has an arrow out
 * of its face. while the set editor holds a wall's task, the wall also
 * gets a blinking title and labels beside the editor's rows */
static void invisible_draw(struct task *task, Uint16 kind)
{
    struct task_work *work = task->work;
    Float x;
    Float y;
    Float z;
    NJS_POINT3 edge;
    NJS_POINT3 from;
    NJS_POINT3 to;
    int i;

    x = work->scl.x + MARGIN;
    y = work->scl.y + MARGIN;
    z = work->scl.z + MARGIN;
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    switch (kind) {
    case DRAW_SPHERE:
        njScale(NULL, x * MARKER_FIT, x * MARKER_FIT, x * MARKER_FIT);
        njEasyDrawObject(&marker_object);
        break;
    case DRAW_CYLINDER:
        njScale(NULL, x * MARKER_FIT, y * MARKER_FIT, x * MARKER_FIT);
        njEasyDrawObject(&marker_cylinder_object);
        break;
    case DRAW_BOX:
        njScale(NULL, x * CUBE_FIT, y * CUBE_FIT, z * CUBE_FIT);
        njEasyDrawObject(marker_cube_object);
        break;
    case DRAW_WALL:
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, 0.0F, 10.0F);
        njScale(NULL, 0.1F, 0.1F, 2.0F);
        njEasyDrawObject(marker_cube_object);
        njPopMatrix(1);
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, 0.0F, 20.0F);
        njRotateX(NULL, NJM_DEG_ANG(45));
        njTranslate(NULL, 0.0F, 0.0F, -3.0F);
        njScale(NULL, 0.1F, 0.1F, 0.7F);
        njEasyDrawObject(marker_cube_object);
        njPopMatrix(1);
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, 0.0F, 20.0F);
        njRotateX(NULL, NJM_DEG_ANG(-45));
        njTranslate(NULL, 0.0F, 0.0F, -3.0F);
        njScale(NULL, 0.1F, 0.1F, 0.7F);
        njEasyDrawObject(marker_cube_object);
        njPopMatrix(1);
        njScale(NULL, x * CUBE_FIT, y * CUBE_FIT, 0.1F);
        njEasyDrawObject(marker_cube_object);
        if (placement_editing(task)) {
            if (work->tick > 25) {
                njPrintC(NJM_LOCATION(8, 8), "< WALL >");
            } else {
                njPrintC(NJM_LOCATION(8, 8), "<      >");
            }
            if (work->tick++ > 50) {
                work->tick = 0;
            }
            njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_X), "->             ");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROT Y       ");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "->             ");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> YOKO        ");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "-> TATE        ");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "->             ");
        }
        break;
    case DRAW_BOUND:
        edge.x = x;
        edge.y = 0.0F;
        edge.z = 0.0F;
        for (i = 0; i < BOUND_SEGMENTS; i++) {
            njPushMatrix(&_unit_matrix_);
            TURN(njRotateY, i * BOUND_TURN);
            njCalcPoint(NULL, &edge, &from);
            njPopMatrix(1);
            njPushMatrix(&_unit_matrix_);
            TURN(njRotateY, (i + 1) * BOUND_TURN);
            njCalcPoint(NULL, &edge, &to);
            njPopMatrix(1);
            draw_line_v(&from, &to);
        }
        break;
    }
    njPopMatrix(1);
}

/* size the record's first shape to the task's scale plus MARGIN, and set
 * the record's radius to match */
static void invisible_set_size(struct task_work *work)
{
    struct collision_shape *shape = work->collision->shapes;

    shape->radius = work->scl.x + MARGIN;
    shape->size_y = work->scl.y + MARGIN;
    shape->size_z = work->scl.z + MARGIN;
    collision_set_radius(work->collision);
}

/* the steps from here to invisible_bound_step test the task's range, the
 * bound's by placement_try_leave_by and the others' by
 * placement_try_leave, and ignore the result. a task found out of range
 * still registers its record this step, and is destroyed on the next.
 * ottotto_step and the steps after it return as soon as their test finds
 * the task out of range */

void invisible_sphere_step(struct task *task)
{
    struct task_work *work = task->work;

    if (edit_mode) {
        invisible_draw(task, DRAW_SPHERE);
    } else {
        /* @hack redundant: the range test's result, tested and left unused,
         * which keeps the image's TST R0,R0 at 0x8c02e8ac; as a bare call,
         * 35 of 51 words differ and invisible.c's code runs 32 bytes short */
        if (placement_try_leave(task)) {
        }
        switch (work->mode) {
        case INVISIBLE_START:
            collision_init(task, &invisible_sphere_shape, COLLISION_OBJECT, 1,
                           1);
            invisible_set_size(work);
            work->mode = INVISIBLE_RUN;
            break;
        case INVISIBLE_RUN:
            collision_register(work);
            break;
        default:
            work->mode = INVISIBLE_START;
            break;
        }
    }
}

void invisible_cylinder_step(struct task *task)
{
    struct task_work *work = task->work;

    if (edit_mode) {
        invisible_draw(task, DRAW_CYLINDER);
    } else {
        /* @hack redundant: the range test's result, tested and left unused,
         * which keeps the image's TST R0,R0 at 0x8c02e912; as a bare call,
         * 35 of 51 words differ and invisible.c's code runs 32 bytes short */
        if (placement_try_leave(task)) {
        }
        switch (work->mode) {
        case INVISIBLE_START:
            collision_init(task, &invisible_cylinder_shape, COLLISION_OBJECT, 1,
                           1);
            invisible_set_size(work);
            work->mode = INVISIBLE_RUN;
            break;
        case INVISIBLE_RUN:
            collision_register(work);
            break;
        default:
            work->mode = INVISIBLE_START;
            break;
        }
    }
}

void invisible_box_step(struct task *task)
{
    struct task_work *work = task->work;

    if (edit_mode) {
        invisible_draw(task, DRAW_BOX);
    } else {
        /* @hack redundant: the range test's result, tested and left unused,
         * which keeps the image's TST R0,R0 at 0x8c02e9b6; as a bare call,
         * 33 of 51 words differ */
        if (placement_try_leave(task)) {
        }
        switch (work->mode) {
        case INVISIBLE_START:
            collision_init(task, &invisible_box_shape, COLLISION_OBJECT, 1, 1);
            invisible_set_size(work);
            work->mode = INVISIBLE_RUN;
            break;
        case INVISIBLE_RUN:
            collision_register(work);
            break;
        default:
            work->mode = INVISIBLE_START;
            break;
        }
    }
}

void invisible_wall_step(struct task *task)
{
    struct task_work *work = task->work;

    if (edit_mode) {
        invisible_draw(task, DRAW_WALL);
    } else {
        /* @hack redundant: the range test's result, tested and left unused,
         * which keeps the image's TST R0,R0 at 0x8c02ea1c; as a bare call,
         * 18 of 51 words differ */
        if (placement_try_leave(task)) {
        }
        switch (work->mode) {
        case INVISIBLE_START:
            collision_init(task, &invisible_wall_shape, COLLISION_WALL, 1, 1);
            invisible_set_size(work);
            work->mode = INVISIBLE_RUN;
            break;
        case INVISIBLE_RUN:
            collision_register(work);
            break;
        default:
            work->mode = INVISIBLE_START;
            break;
        }
    }
}

void invisible_bound_step(struct task *task)
{
    struct task_work *work = task->work;

    if (edit_mode) {
        invisible_draw(task, DRAW_BOUND);
    } else {
        /* @hack redundant: the range test's result, tested and left unused,
         * which keeps the image's TST R0,R0 at 0x8c02eaba; as a bare call,
         * 33 of 53 words differ */
        if (placement_try_leave_by(task, BOUND_RANGE)) {
        }
        switch (work->mode) {
        case INVISIBLE_START:
            collision_init(task, &invisible_bound_shape, COLLISION_WALL, 1, 1);
            invisible_set_size(work);
            work->mode = INVISIBLE_RUN;
            break;
        case INVISIBLE_RUN:
            collision_register(work);
            break;
        default:
            work->mode = INVISIBLE_START;
            break;
        }
    }
}

/* draw the box as invisible_draw does, then a wall's arrowheads and face
 * without its shaft. while the set editor holds the task, it also shows a
 * blinking title and labels beside the editor's rows */
static void ottotto_draw(struct task *task)
{
    struct task_work *work = task->work;
    Float x;
    Float y;
    Float z;

    x = work->scl.x + MARGIN;
    y = work->scl.y + MARGIN;
    z = work->scl.z + MARGIN;
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    njPushMatrix(NULL);
    njScale(NULL, x * CUBE_FIT, y * CUBE_FIT, z * CUBE_FIT);
    njEasyDrawObject(marker_cube_object);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslate(NULL, 0.0F, 0.0F, 20.0F);
    njRotateX(NULL, NJM_DEG_ANG(45));
    njTranslate(NULL, 0.0F, 0.0F, -3.0F);
    njScale(NULL, 0.1F, 0.1F, 0.7F);
    njEasyDrawObject(marker_cube_object);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslate(NULL, 0.0F, 0.0F, 20.0F);
    njRotateX(NULL, NJM_DEG_ANG(-45));
    njTranslate(NULL, 0.0F, 0.0F, -3.0F);
    njScale(NULL, 0.1F, 0.1F, 0.7F);
    njEasyDrawObject(marker_cube_object);
    njPopMatrix(1);
    njScale(NULL, x * CUBE_FIT, y * CUBE_FIT, 0.1F);
    njEasyDrawObject(marker_cube_object);
    if (placement_editing(task)) {
        if (work->tick > 25) {
            njPrintC(NJM_LOCATION(8, 8), "< OTTOTTO >");
        } else {
            njPrintC(NJM_LOCATION(8, 8), "<      >");
        }
        if (work->tick++ > 50) {
            work->tick = 0;
        }
        njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_X), "->             ");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROT Y       ");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "->             ");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> YOKO        ");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "-> TATE        ");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "-> O OKU3      ");
    }
    njPopMatrix(1);
}

void ottotto_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (edit_mode) {
        ottotto_draw(task);
    } else {
        switch (work->mode) {
        case INVISIBLE_START:
            collision_init(task, &ottotto_shape, COLLISION_OBJECT, 1, 1);
            invisible_set_size(work);
            work->mode = INVISIBLE_RUN;
            work->id = OTTOTTO_ID;
            break;
        case INVISIBLE_RUN:
            collision_register(work);
            break;
        default:
            work->mode = INVISIBLE_START;
            break;
        }
    }
}

void invisible_jump_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (edit_mode) {
        invisible_draw(task, DRAW_BOX);
    } else {
        switch (work->mode) {
        case INVISIBLE_START:
            collision_init(task, &ottotto_shape, COLLISION_OBJECT, 1, 1);
            invisible_set_size(work);
            work->mode = INVISIBLE_RUN;
            work->id = JUMP_BOX_ID;
            break;
        case INVISIBLE_RUN:
            collision_register(work);
            break;
        default:
            work->mode = INVISIBLE_START;
            break;
        }
    }
}

/* TRUE when the shape's center, moved the shape's radius along each of the
 * box's axes, lies in the box. the box is centered on work's position and
 * turned by its angles, and its half sizes are its scale. it returns FALSE
 * early when the center is farther from the position than the scale's
 * length plus the radius */
static Bool invisible_box_holds(struct collision_shape *shape,
                                struct task_work *work)
{
    NJS_POINT3 center;
    NJS_POINT3 pos;
    NJS_VECTOR offset;
    Float range;
    Float x;
    Float y;
    Float z;
    Float radius;
    Float half_x;
    Float half_y;
    Float half_z;

    center = shape->center;
    pos = work->pos;
    range = njScalor(&work->scl) + shape->radius;
    /* @hack spelling: range on the left of the <, which keeps it in FR12
     * across njDistanceP2P (0x8c02eee0); the other way round, SHC reloads
     * it from the frame after the call and 42 of 148 words differ */
    if (range < njDistanceP2P(&center, &pos)) {
        return FALSE;
    }
    offset.x = center.x - pos.x;
    offset.y = center.y - pos.y;
    offset.z = center.z - pos.z;
    matrix_push_unit();
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    njInvertMatrix(NULL);
    njCalcPoint(NULL, &offset, &offset);
    njPopMatrix(1);
    radius = shape->radius;
    x = offset.x + radius;
    y = offset.y + radius;
    z = offset.z + radius;
    half_x = njAbs(work->scl.x);
    half_y = njAbs(work->scl.y);
    half_z = njAbs(work->scl.z);
    if (x <= half_x && x >= -half_x && y <= half_y && y >= -half_y &&
        z <= half_z && z >= -half_z) {
        return TRUE;
    }
    return FALSE;
}

void invisible_dry_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player;
    struct collision_work *collision;
    int i;

    if (placement_try_leave(task)) {
        return;
    }
    if (edit_mode) {
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateZ, work->ang.z);
        TURN(njRotateY, work->ang.y);
        TURN(njRotateX, work->ang.x);
        njScale(NULL, work->scl.x * CUBE_FIT, work->scl.y * CUBE_FIT,
                work->scl.z * CUBE_FIT);
        njEasyDrawObject(marker_cube_object);
        njPopMatrix(1);
        return;
    }
    switch (work->mode) {
    case INVISIBLE_START:
        work->mode = INVISIBLE_RUN;
        break;
    case INVISIBLE_RUN:
        for (i = 1; i >= 0; i--) {
            player = player_work[i];
            if (player == NULL) {
                continue;
            }
            collision = player->collision;
            if (collision == NULL) {
                continue;
            }
            if (invisible_box_holds(collision->shapes, work)) {
                player_get(i)->shadow.above_flags &= ~WORLD_LIQUID;
            }
        }
        if (camera_work != NULL) {
            invisible_camera_shape->center = camera_work->pos;
            if (invisible_box_holds(invisible_camera_shape, work)) {
                camera_in_dry_box = TRUE;
            }
        }
        break;
    default:
        work->mode = INVISIBLE_START;
        break;
    }
}
