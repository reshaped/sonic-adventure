/* what the Chaos fights share: the boss's record and setup, the puddle he
 * flows as, his water's animation, his blink, and small helpers */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "chaos/chaos.h"
#include "chaos/emeralds.h"
#include "chaos/models.h"
#include "chaos/puddle.h"
#include "chaos/steer.h"
#include "collision/ground.h"
#include "draw/motion.h"
#include "draw/pose.h"
#include "player/player.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/util.h"

/* the deepest chaos_list_nodes descends, unchecked */
#define LIST_DEPTH 32

#define TRAIL_STEPS 18
/* the puddle's rim points, where chaos_follow_ground finds the ground, and
 * the angle from each to the next */
#define RIM_POINTS 8
#define RIM_SPAN NJM_DEG_ANG(45)
/* the most chaos_follow_ground moves a height toward the ground in a call */
#define GROUND_EASE 0.1F

/* the frames of CHAOS_SURFACE's water, and the calls of
 * chaos_animate_surface that each frame shows for */
#define SURFACE_FRAMES 6
#define SURFACE_CALLS 4

enum tether_mode { TETHER_INIT, TETHER_HOLD };

/* a tether's vars: the point it keeps within scl.x of the work's place */
struct tether_vars {
    NJS_POINT3 *point;
};

#define TETHER(work) ((struct tether_vars *)&(work)->vars)

/* a point of the puddle's model, as PUDDLE_MAKE keeps it */
struct puddle_point {
    NJS_POINT3 pos;
    Angle ang; /* its angle round the model's middle, across */
    /* the share of the point's height that the ground under the rim gives
     * it: a fifth of its distance across from the middle. the ground under
     * the middle gives the rest */
    Float weight;
};

/* each form's puddle and blob texture, as an index in his texlist */
static const Sint16 chaos_form_textures[CHAOS_FORMS] = {2, 8, 8, 15, 0, 0};

/* the indexes of the puddle model's rim points */
static Sint16 puddle_rim[RIM_POINTS] = {
#include "chaos/chaos/puddle_rim.inc"
};
/* the ends of chaos_draw_axes' lines, -1 and 1 on each axis */
static NJS_POINT3 chaos_axis_lines[6] = {
#include "chaos/chaos/chaos_axis_lines.inc"
};
/* the white chaos_draw_axes draws in when it is given no color */
static NJS_COLOR chaos_axis_color = {
#include "chaos/chaos/chaos_axis_color.inc"
};
/* the x, y and z axes, indexed by a mark's axis. chaos_track_points turns
 * the one a mark picks by the mark's node */
static NJS_VECTOR chaos_axes[3] = {
    {1.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F}, {0.0F, 0.0F, 1.0F}};
/* the white a blinking node shows in chaos_set_material */
static NJS_ARGB chaos_blink_color = {
#include "chaos/chaos/chaos_blink_color.inc"
};

/* the puddle's last places, latest first, which its points trail behind */
static NJS_POINT3 puddle_trail[TRAIL_STEPS];
/* each rim point's height, eased toward the ground under it. the last slot
 * holds the ground's height under the middle. each is measured from the
 * work's y */
static Float puddle_ground[RIM_POINTS + 1];
static Float puddle_highest; /* the highest rim height, which nothing reads */
/* chaos_form_textures' texture for the form that fights */
static Sint16 puddle_texture;
static struct puddle_point *puddle_points;

int chaos_list_nodes(NJS_OBJECT *object, NJS_OBJECT **list)
{
    NJS_OBJECT *stack[LIST_DEPTH];
    int count;
    int depth;
    Bool walking;
    Bool climbing;

    /* @hack spelling: depth stepped by assignment, depth + 1 and depth - 1,
     * which loads the 0 into R7 and copies it on (0x8c0fbc6e to 0x8c0fbc74);
     * as ++ and --, R6 and R7 trade and 12 of 52 words differ */
    count = 0;
    depth = 0;
    walking = TRUE;
    do {
        *list++ = object;
        count++;
        if (object->child != NULL) {
            stack[depth] = object;
            object = object->child;
            depth = depth + 1;
        } else {
            climbing = TRUE;
            do {
                if (object->sibling != NULL) {
                    object = object->sibling;
                    climbing = FALSE;
                } else if (depth > 0) {
                    depth = depth - 1;
                    object = stack[depth];
                } else {
                    climbing = FALSE;
                    walking = FALSE;
                }
            } while (climbing);
        }
    } while (walking);
    *list = NULL;
    return count;
}

void chaos_setup(struct task_work *work, struct chaos *chaos, int form)
{
    chaos_current_form = form;
    chaos_travel = 0.0F;
    chaos_params = &chaos_forms[form];
    chaos->unk_3e = 0;
    chaos->figures = 1;
    chaos->scale.x = chaos->scale.y = chaos->scale.z = 1.4F;
    chaos->health = chaos_params->hits;
    chaos->colors[0].a = 0.0F;
    chaos->colors[0].r = 0.0F;
    chaos->colors[0].g = 0.0F;
    chaos->colors[0].b = 0.0F;
    boss_next_mode = work->mode;
    puddle_texture = chaos_form_textures[form];
    /* @hack spelling: each figure reached through its address, (&x)->model,
     * which loads chaos_puddle's address into R3 ahead of a store
     * (0x8c0fbd6a, 0x8c0fbd6c); as chaos_puddle.model, 11 of 82 words
     * differ */
    (&chaos_puddle)->model->mats->attr_texId = puddle_texture;
    (&chaos_blob)->model->mats->attr_texId = puddle_texture;
    work->scl.x = work->scl.y = work->scl.z = 1.0F;
    if (form == CHAOS_4) {
        chaos_face_off(work);
    }
    camera_swing_done = 0;
    chaos_emerald_clear();
}

void chaos_face_off(struct task_work *work)
{
    struct task_work *player = player_work[0];
    Angle ang;

    stage_place_player(player);
    work->pos = chaos_params->start;
    ang = (Angle)(atan2f(work->pos.z - player->pos.z,
                         work->pos.x - player->pos.x) *
                  ANGLE_PER_RADIAN);
    player->ang.y = ang;
    work->ang.y = -ang;
}

/* ease each rim point's height toward the ground under it by at most
 * GROUND_EASE a call. ease the work's y toward the lowest of those grounds
 * by as much. the ground under the middle is taken without easing */
static void chaos_follow_ground(struct task_work *work, NJS_POINT3 *points)
{
    Float heights[RIM_POINTS];
    struct angles slope;
    Float lowest = 99999.0F;
    Float x;
    Float z;
    Float eased;
    int i;

    puddle_highest = -99999.0F;
    for (i = 0; i < RIM_POINTS; i++) {
        x = work->pos.x + points[puddle_rim[i]].x;
        z = work->pos.z + points[puddle_rim[i]].z;
        heights[i] =
            ground_below(x, work->pos.y + 3.0F, z, &slope) - work->pos.y;
        lowest = heights[i] < lowest ? heights[i] : lowest;
        puddle_ground[i] +=
            CLAMP(heights[i] - puddle_ground[i], -GROUND_EASE, GROUND_EASE);
        eased = puddle_ground[i];
        puddle_highest = eased > puddle_highest ? eased : puddle_highest;
    }
    work->pos.y += CLAMP(lowest, -GROUND_EASE, GROUND_EASE);
    puddle_ground[RIM_POINTS] =
        ground_below(work->pos.x, work->pos.y + 3.0F, work->pos.z, &slope) -
        work->pos.y;
}

void chaos_puddle_deform(struct task_work *work, struct chaos *chaos,
                         NJS_OBJECT *object)
{
    Sint32 count = object->model->nbPoint;
    int i;
    int index;
    int next;
    Angle ang;
    NJS_POINT3 *point;
    NJS_POINT3 *origin;
    NJS_POINT3 *normal;
    struct puddle_point *kept;
    /* @hack redundant: register on height, which puts it ahead of count in
     * SHC's weighing, with object in R8 and work in R13 (0x8c0fc04a,
     * 0x8c0fc050); as a plain Float, they take R13 and R12, the function runs
     * 4 bytes short and 295 of 410 words differ */
    register Float height;

    switch (chaos->puddle_mode) {
    case PUDDLE_MAKE:
        puddle_points = heap_alloc(count * sizeof(struct puddle_point));
        point = object->model->points;
        /* @hack spelling: i = 0 ahead of kept, which stores puddle_points
         * before the first test (0x8c0fc0ae to 0x8c0fc0b2); the other way
         * round, 12 of 410 words differ */
        i = 0;
        kept = puddle_points;
        for (; i < count; kept++) {
            kept->ang = (Angle)(atan2f(point->z, point->x) * ANGLE_PER_RADIAN);
            kept->weight =
                njSqrt((point->x * point->x) + (point->z * point->z)) * 0.2F;
            kept->pos = *point++;
            /* @hack spelling: i's step after the stores, which keeps
             * ANGLE_PER_RADIAN in FR13 (0x8c0fc0b8); ahead of them, FR12, and
             * 125 of 410 words differ */
            /* @hack spelling: i = i + 1, which SHC weighs twice and so keeps
             * the constant 0 in R11 (0x8c0fc064); as i++, R12, and 23 of 410
             * words differ */
            i = i + 1;
        }
        /* fall through */
    case PUDDLE_RESET:
        for (i = 0; i < TRAIL_STEPS; i++) {
            puddle_trail[i] = work->pos;
        }
        point = object->model->points;
        /* @hack spelling: i = 0 ahead of kept, which keeps R11 for the constant
         * 0 and R12 for PUDDLE_MAKE's point (0x8c0fc064, 0x8c0fc0b2); the other
         * way round, the two trade and 38 of 410 words differ */
        i = 0;
        kept = puddle_points;
        /* @hack spelling: i = i + 1, which keeps PUDDLE_MAKE's
         * ANGLE_PER_RADIAN in FR13 (0x8c0fc0b8); as i++, FR12, and 42 of 410
         * words differ */
        for (; i < count; i = i + 1) {
            *point++ = kept->pos;
            kept++;
        }
        chaos->puddle_mode = PUDDLE_FLOW;
        break;
    case PUDDLE_FLOW:
        /* @hack redundant: ang set from work and never read, whose reference
         * keeps R11 for the constant 0 and R12 for PUDDLE_MAKE's point
         * (0x8c0fc064, 0x8c0fc0b2); without it, the two trade and 45 of 410
         * words differ */
        ang = work->ang.y;
        for (i = TRAIL_STEPS - 1; i > 0; i--) {
            puddle_trail[i] = puddle_trail[i - 1];
        }
        puddle_trail[0] = work->pos;
        origin = &puddle_trail[0];
        point = object->model->points;
        /* @hack spelling: i = 0 ahead of kept, which stores R11 to the frame
         * before the loop loads count (0x8c0fc20c, 0x8c0fc20e); the other way
         * round, 5 of 410 words differ */
        i = 0;
        kept = puddle_points;
        for (; i < count; i++) {
            ang = angle_difference(kept->ang, work->ang.y);
            ang = (Uint16)(ang < 0 ? -ang : ang);
            if (ang >= NJM_DEG_ANG(90)) {
                ang = (ang - NJM_DEG_ANG(90)) * TRAIL_STEPS / NJM_DEG_ANG(90);
                index = CLAMP(ang, 0, TRAIL_STEPS - 1);
            } else {
                index = 0;
            }
            point->x = kept->pos.x + puddle_trail[index].x - origin->x;
            point->z = kept->pos.z + puddle_trail[index].z - origin->z;
            point++;
            kept++;
        }
        chaos_follow_ground(work, object->model->points);
        kept = puddle_points;
        point = object->model->points;
        i = 0;
#ifdef GCC
        /* repairs the unset normal the tag below names: the image reads the
         * normals of chaos_puddle's 376 points from the 4512 bytes of bss at
         * &puddle_points. past the pointer they are other sources' statics,
         * which GCC lays out otherwise. GCC reads the model's own normals,
         * one for each point */
        normal = object->model->normals;
#endif
        /* @hack spelling: the steps in both arms, which SHC merges from
         * point->y's store on, and which keep R11 for the constant 0 and R12
         * for PUDDLE_MAKE's point (0x8c0fc064, 0x8c0fc0b2); one step after the
         * if, the two trade and 36 of 410 words differ */
        /* @hack spelling: i = i + 1 in the first arm, which keeps i in R13
         * (MOV R11,R13 at 0x8c0fc2d8); with i++ in both, i takes R12 and 58
         * of 410 words differ */
        while (i < count) {
            ang = kept->ang;
            index = (ang / RIM_SPAN) & (RIM_POINTS - 1);
            next = (index + 1) & (RIM_POINTS - 1);
            ang &= RIM_SPAN - 1;
            /* @bug memory: no path sets normal, which is R9,
             * still &puddle_points from 0x8c0fc052: normal->y reads the float
             * 4 bytes past it at 0x8c0fc30c, and 12 further at each point */
            /* NOLINTNEXTLINE(clang-diagnostic-uninitialized) */
            if (normal->y > 0.01F) {
                height = (((RIM_SPAN - ang) * puddle_ground[index]) +
                          (ang * puddle_ground[next])) *
                         (1.0F / RIM_SPAN);
                height = (height * kept->weight) +
                         (puddle_ground[RIM_POINTS] * (1.0F - kept->weight));
                point->y = kept->pos.y + height;
                normal++;
                point++;
                kept++;
                i = i + 1;
            } else {
                point->y = kept->pos.y;
                normal++;
                point++;
                kept++;
                i++;
            }
        }
        break;
    case PUDDLE_FREE:
        point = object->model->points;
        /* @hack spelling: i = 0 ahead of kept, which keeps R11 for the constant
         * 0 and R12 for PUDDLE_MAKE's point (0x8c0fc064, 0x8c0fc0b2); the other
         * way round, the two trade and 25 of 410 words differ */
        i = 0;
        kept = puddle_points;
        /* @hack spelling: i = i + 1, which loads puddle_ground's address into
         * R10 ahead of the MOVA at 0x8c0fc2d4 (0x8c0fc2d2); as i++, the two
         * trade and 18 of 410 words differ */
        for (; i < count; i = i + 1) {
            *point++ = kept->pos;
            kept++;
        }
        heap_free(puddle_points);
        break;
    }
}

void chaos_draw_axes(Float scale, NJS_COLOR *color)
{
    NJS_COLOR colors[6];
    NJS_POINT3COL lines;
    int i;

    lines.p = chaos_axis_lines;
    lines.tex = NULL;
    lines.num = 3;
    lines.col = colors;
    if (color == NULL) {
        color = &chaos_axis_color;
    }
    for (i = 0; i < 6; i++) {
        colors[i] = *color;
    }
    njPushMatrix(NULL);
    njScale(NULL, scale, scale, scale);
    njDrawLine3D(&lines, 3, NJD_TRANSPARENT);
    njPopMatrix(1);
}

void chaos_track_points(struct task_work *work, struct boss_mark *marks,
                        Float offset)
{
    struct boss_point *point = boss_points;
    NJS_MATRIX *matrix;
    int node;

    matrix = heap_alloc(sizeof(NJS_MATRIX));
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    if (chaos_current_form == CHAOS_6) {
        TURN(njRotateY, NJM_DEG_ANG(90) - work->ang.y);
    } else {
        TURN(njRotateY, NJM_DEG_ANG(180) - work->ang.y);
    }
    njTranslate(NULL, 0.0F, offset, 0.0F);

    while ((node = (int)marks->node) >= 0) {
        point->last = point->pos;
        njPushMatrix(NULL);
        pose_get_matrix(node, matrix);
        njMultiMatrix(NULL, matrix);
        njCalcPoint(NULL, &marks->pos, &point->pos);
        njCalcVector(NULL, &chaos_axes[marks->axis], &point->axis);
        njPopMatrix(1);
        marks++;
        point++;
    }
    njPopMatrix(1);
    heap_free(matrix);
}

struct task *chaos_create_task(int level, void (*step)(struct task *task),
                               struct task *source, NJS_POINT3 *offset)
{
    struct task *task = task_create(TASK_WORK, level, step);

    if (task != NULL) {
        task->work->pos = source->work->pos;
        if (offset != NULL) {
            njAddVector(&source->work->pos, offset);
        }
    }
    return task;
}

struct task *chaos_create_child(Uint16 flags, void (*step)(struct task *task),
                                struct task *parent, NJS_POINT3 *offset)
{
    struct task *task = task_create_child(flags, step, parent);

    if (task != NULL) {
        task->work->pos = parent->work->pos;
        if (offset != NULL) {
            njAddVector(&parent->work->pos, offset);
        }
    }
    return task;
}

void chaos_animate_surface(struct chaos *chaos)
{
    NJS_TEXNAME *texture = &chaos->texlist->textures[puddle_texture];

    chaos->tick++;
    if (chaos->tick >= SURFACE_CALLS) {
        chaos->tick = 0;
        chaos->frame++;
        if (chaos->frame >= SURFACE_FRAMES) {
            chaos->frame = 0;
        }
    }
    texture->texaddr = chaos_surface_textures[chaos->frame].texaddr;
}

void chaos_set_material(NJS_OBJECT *object)
{
    struct chaos *chaos;
    NJS_MATERIAL *material;

    if (object->model != NULL) {
        material = object->model->mats;
        chaos = chaos_record;
        if (material->attrflags & MATERIAL_BLINKS) {
            switch (chaos->blink) {
            /* @hack redundant: case 0, which keeps the image's CMP/EQ #0 at
             * 0x8c0fc72c; without it, 23 of 56 words differ */
            case 0:
                break;
            case 1:
                if (chaos->steps & 1) {
                    njSetConstantMaterial(&chaos_blink_color);
                }
                break;
            /* @hack redundant: case 2, which keeps the image's CMP/EQ #2 at
             * 0x8c0fc734; without it, 21 of 56 words differ */
            case 2:
                break;
            }
            chaos->flags |= CHAOS_BLINKED;
            return;
        }
        if (chaos->flags & CHAOS_BLINKED) {
            chaos->flags &= ~CHAOS_BLINKED;
            njSetConstantMaterial(&chaos->colors[0]);
        }
    }
}

/* a point that strays further than scl.x across from the work goes back
 * onto that circle. there it rises to the ground if it is below it */
static void tether_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 *point;
    struct angles slope;
    Float dx;
    Float dz;
    Float distance;
    Float ground;
    Angle ang;

    switch (work->mode) {
    case TETHER_INIT:
        work->mode = TETHER_HOLD;
        /* fall through */
    case TETHER_HOLD:
        point = TETHER(work)->point;
        dx = point->x - work->pos.x;
        dz = point->z - work->pos.z;
        /* @hack spelling: the distance through a local, which copies njSqrt's
         * result to FR4 (FMOV.S FR0,FR4 at 0x8c0fc828); in the test, it is
         * compared in FR0, the function runs 2 bytes short and 61 of 95 words
         * differ */
        distance = njSqrt((dx * dx) + (dz * dz));
        if (distance > work->scl.x) {
            ang = (Angle)(atan2f(dz, dx) * ANGLE_PER_RADIAN);
            point->x = work->pos.x + (work->scl.x * njCos(ang));
            point->z = work->pos.z + (work->scl.x * njSin(ang));
            ground =
                ground_below(point->x, work->pos.y + 10.0F, point->z, &slope);
            point->y = point->y > ground ? point->y : ground;
        }
        break;
    }
}

struct task *chaos_tether_create(NJS_POINT3 *point, NJS_POINT3 *center,
                                 Float radius)
{
    struct task *task = task_create(TASK_WORK, 0, tether_step);
    struct task_work *work;

    if (task != NULL) {
        work = task->work;
        work->pos = *center;
        work->scl.x = radius;
        TETHER(work)->point = point;
    }
    return task;
}

void chaos_draw_begin(void)
{
    NJS_ARGB color;

    njControl3D(NJD_CONTROL_3D_CONSTANT_TEXTURE_MATERIAL |
                NJD_CONTROL_3D_MODEL_CLIP | NJD_CONTROL_3D_DEPTH_QUEUE |
                NJD_CONTROL_3D_OFFSET_MATERIAL | NJD_CONTROL_3D_ENABLE_ALPHA |
                NJD_CONTROL_3D_CONSTANT_ATTR);
    njSetConstantAttr(0xffffffff, NJD_FILTER_BILINEAR);
    color.a = color.r = color.g = color.b = 0.0F;
    njSetConstantMaterial(&color);
}

Float chaos_distance_across(NJS_POINT3 *from, NJS_POINT3 *to)
{
    Float dx = from->x - to->x;
    Float dz = from->z - to->z;

    return njSqrt((dx * dx) + (dz * dz));
}

Float chaos_distance_across_squared(NJS_POINT3 *from, NJS_POINT3 *to)
{
    Float dx = from->x - to->x;
    Float dz = from->z - to->z;

    return (dx * dx) + (dz * dz);
}
