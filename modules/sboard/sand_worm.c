/* the sand worms, which leap out of the sand down four paths while a
 * player is near. each leap sends the head, three bodies and the tail down
 * the path, one after another. each segment moves a point a step, and what
 * stands on it rides along. sand smoke rises where the worm breaks the
 * sand */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/world.h"
#include "draw/tree.h"
#include "object/simple.h"
#include "player/player.h"
#include "sboard/act.h"
#include "sboard/rumble.h"
#include "sboard/sand_smoke.h"
#include "sboard/sand_worm.h"
#include "sound/cue.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/task_desc.h"
#include "system/texture.h"
#include "system/util.h"

/* where a leaping worm breaks the sand. start and finish count steps since
 * the leap made its head, as the record's steps does. at start, the worm
 * clears done and raises sand smoke with shape at points[point]. at finish,
 * it sets done, and the smoke fades out */
struct sand_worm_breach {
    Sint16 start;
    Sint16 finish;
    Sint16 point;
    Sint8 done;
    struct sand_smoke_shape shape;
};

/* a worm's path. the segments start at points[first], and go as far as
 * points[count - 1 - trim]. worms_left is how many more worms the path may
 * take. a worm's init takes one, and its destroy gives it back. a worm made
 * with none left destroys itself on its next step */
struct sand_worm_path {
    Sint8 worms_left;
    Sint8 segments; /* how many a leap makes, the head and tail included */
    Sint8 first;
    Sint8 trim;
    const NJS_POINT3 *points;
    int count;
    Float scale; /* the worm's size */
    /* the steps the worm skips between making two segments. gaps[0]
     * follows the head, gaps[1] each body but the last, and gaps[2] the
     * last body */
    Sint8 gaps[3];
    struct sand_worm_breach *breaches;
    Sint8 breach_count;
};

/* a worm's record, in task->motion. init copies the path's fields here,
 * with last the index of the segments' last point */
struct sand_worm_record {
    Sint8 mode;
    Sint8 segments;
    Sint8 made; /* the segments this leap has made */
    Sint8 wait; /* the steps to skip before making the next */
    Sint16 first;
    Sint16 last;
    const NJS_POINT3 *points;
    int count;
    Float scale;
    struct sand_worm_path *path;
    Sint8 *gaps;
    /* points[(last - first) / 2], which the players' range and the rumble
     * are measured from */
    const NJS_POINT3 *middle;
    /* the steps since the leap made its head, which sand_worm_breach checks
     * each breach's start and finish against. it is 0 while the worm has no
     * children */
    Sint16 steps;
    struct sand_worm_breach *breaches;
    Sint8 breach_count;
};

enum sand_worm_mode { SAND_WORM_WAIT, SAND_WORM_LEAP };

/* a segment's record, in task->motion */
struct sand_worm_segment {
    Sint8 mode;
    Sint16 point; /* the index of the path point it is at */
    Float reach; /* the world test's radius at scale 1: twice the model's */
    NJS_OBJECT *object;
    NJS_OBJECT *world; /* the segment's world object */
};

/* a player within WORM_NEAR of the path's middle starts the worm's leaps.
 * once no player is within WORM_FAR, the worm destroys its children and
 * waits again */
#define WORM_NEAR 1200.0F
#define WORM_FAR 2400.0F

/* a leaping worm rumbles the pad while the path's middle is within
 * RUMBLE_REACH of the camera */
#define RUMBLE_REACH 500.0F

#define SE_SAND_WORM_BREACH 692

extern NJS_OBJECT sand_worm_head_object;
extern NJS_OBJECT sand_worm_body_object;
extern NJS_OBJECT sand_worm_tail_object;

static void sand_worm_init(struct task *task, void *arg);
static void sand_worm_step(struct task *task);
static void sand_worm_destroy(struct task *task);
static void sand_worm_segment_init(struct task *task, void *arg);
static void sand_worm_segment_step(struct task *task);
static void sand_worm_segment_draw(struct task *task);
static void sand_worm_segment_destroy(struct task *task);
static void sand_worm_segment_follow(struct task *task);
static void sand_worm_segment_move(NJS_POINT3 *pos, NJS_VECTOR *speed,
                                   const NJS_POINT3 *point);
static void sand_worm_segment_turn(struct angles *ang, struct angles *turn,
                                   const NJS_POINT3 *point, int index,
                                   int count);
static void sand_worm_breach(struct task *task);

static const NJS_POINT3 sand_worm_a_points[199] = {
#include "sboard/sand_worm/sand_worm_a_points.inc"
};

static const NJS_POINT3 sand_worm_b_points[199] = {
#include "sboard/sand_worm/sand_worm_b_points.inc"
};

static const NJS_POINT3 sand_worm_c_points[199] = {
#include "sboard/sand_worm/sand_worm_c_points.inc"
};

static const NJS_POINT3 sand_worm_d_points[199] = {
#include "sboard/sand_worm/sand_worm_d_points.inc"
};

static const struct task_desc sand_worm_desc = {sizeof(struct sand_worm_record),
                                                0,
                                                sand_worm_init,
                                                sand_worm_step,
                                                NULL,
                                                sand_worm_destroy};
static const struct task_desc sand_worm_segment_desc = {
    sizeof(struct sand_worm_segment), TASK_WORK | TASK_FORCE,
    sand_worm_segment_init,           sand_worm_segment_step,
    sand_worm_segment_draw,           sand_worm_segment_destroy};

/* the points of a breach's smoke: its center, and the corners of a hexagon
 * around it. the hexagon's radius is 250 for the big worms and 62.5 for
 * the small one */
static const NJS_POINT3 sand_worm_smoke[7] = {
    {0.0F, 20.0F, 0.0F},          {250.0F, 20.0F, 0.0F},
    {125.0F, 20.0F, 216.49999F},  {-125.0F, 20.0F, 216.49999F},
    {-250.0F, 20.0F, 0.0F},       {-125.0F, 20.0F, -216.49999F},
    {125.0F, 20.0F, -216.49999F},
};
static const NJS_POINT3 sand_worm_small_smoke[7] = {
    {0.0F, 5.0F, 0.0F},          {62.5F, 5.0F, 0.0F},
    {31.25F, 5.0F, 54.124997F},  {-31.25F, 5.0F, 54.124997F},
    {-62.5F, 5.0F, 0.0F},        {-31.25F, 5.0F, -54.124997F},
    {31.25F, 5.0F, -54.124997F},
};

static const struct sound_falloff sand_worm_falloff = {63, -128, 100.0F,
                                                       2400.0F};
/* the angles every breach's smoke takes: none, so its hexagon lies level */
static const struct angles sand_worm_smoke_ang = {0, 0, 0};

/* the paths. a, b and c carry big worms, and d a worm a quarter their
 * size */
/* clang-format off */
static struct sand_worm_breach sand_worm_a_breaches[2] = {
    {23, 155, 53, 0, {sand_worm_smoke, 7, 10.0F}},
    {101, 233, 131, 0, {sand_worm_smoke, 7, 10.0F}},
};
static struct sand_worm_path sand_worm_a = {
    1, 5, 0, 0, sand_worm_a_points, 199, 1.0F, {18, 18, 18},
    sand_worm_a_breaches, 2};

static struct sand_worm_breach sand_worm_b_breaches[2] = {
    {24, 156, 54, 0, {sand_worm_smoke, 7, 10.0F}},
    {118, 250, 148, 0, {sand_worm_smoke, 7, 10.0F}},
};
static struct sand_worm_path sand_worm_b = {
    1, 5, 0, 0, sand_worm_b_points, 199, 1.0F, {18, 18, 18},
    sand_worm_b_breaches, 2};

static struct sand_worm_breach sand_worm_c_breaches[2] = {
    {21, 153, 51, 0, {sand_worm_smoke, 7, 10.0F}},
    {107, 239, 137, 0, {sand_worm_smoke, 7, 10.0F}},
};
static struct sand_worm_path sand_worm_c = {
    1, 5, 0, 0, sand_worm_c_points, 199, 1.0F, {18, 18, 18},
    sand_worm_c_breaches, 2};

static struct sand_worm_breach sand_worm_d_breaches[4] = {
    {7, 51, 26, 0, {sand_worm_small_smoke, 7, 2.5F}},
    {42, 86, 61, 0, {sand_worm_small_smoke, 7, 2.5F}},
    {69, 113, 88, 0, {sand_worm_small_smoke, 7, 2.5F}},
    {127, 171, 146, 0, {sand_worm_small_smoke, 7, 2.5F}},
};
static struct sand_worm_path sand_worm_d = {
    1, 5, 0, 0, sand_worm_d_points, 199, 0.25F, {5, 5, 5},
    sand_worm_d_breaches, 4};
/* clang-format on */

struct task *sand_worm_a_create(void)
{
    /* @hack spelling: the result through task, which keeps the call a JSR
     * (0x0c904b68); returned directly, it becomes a tail jump to
     * act_task_create, and 9 of 9 words differ */
    struct task *task = act_task_create(&sand_worm_desc, &sand_worm_a);

    return task;
}

struct task *sand_worm_b_create(void)
{
    /* @hack spelling: the result through task, which keeps the call a JSR
     * (0x0c904b7a); returned directly, it becomes a tail jump to
     * act_task_create, and 9 of 9 words differ */
    struct task *task = act_task_create(&sand_worm_desc, &sand_worm_b);

    return task;
}

struct task *sand_worm_c_create(void)
{
    /* @hack spelling: the result through task, which keeps the call a JSR
     * (0x0c904b8c); returned directly, it becomes a tail jump to
     * act_task_create, and 9 of 9 words differ */
    struct task *task = act_task_create(&sand_worm_desc, &sand_worm_c);

    return task;
}

struct task *sand_worm_d_create(void)
{
    /* @hack spelling: the result through task, which keeps the call a JSR
     * (0x0c904b9e); returned directly, it becomes a tail jump to
     * act_task_create, and 9 of 9 words differ */
    struct task *task = act_task_create(&sand_worm_desc, &sand_worm_d);

    return task;
}

static void sand_worm_init(struct task *task, void *arg)
{
    struct sand_worm_record *record = (struct sand_worm_record *)task->motion;
    struct sand_worm_path *path = arg;
    int first;
    int last;
    int end;
    const NJS_POINT3 *points;

    record->path = path;
    record->path->worms_left--;
    if (path->worms_left < 0) {
        task_kill(task);
        return;
    }
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    first = path->first;
    end = path->count - path->trim;
    last = end - 1;
    points = path->points;
    record->mode = SAND_WORM_WAIT;
    record->segments = path->segments;
    record->made = 0;
    record->wait = 0;
    record->first = first;
    record->last = last;
    record->points = points;
    record->count = path->count;
    record->scale = path->scale;
    record->gaps = path->gaps;
    record->middle = &points[(last - first) / 2];
    record->steps = 0;
    record->breaches = path->breaches;
    record->breach_count = path->breach_count;
}

/* a leap makes the head, the bodies and the tail as children, skipping
 * gaps steps between them. the next leap starts once all the worm's
 * children are gone, its smoke included. while it has children, the worm
 * counts the leap's steps for its breaches, and rumbles the pad */
static void sand_worm_step(struct task *task)
{
    struct sand_worm_record *record = (struct sand_worm_record *)task->motion;
    NJS_OBJECT *object;
    int made;
    int last;
    Sint8 *gaps;

    switch (record->mode) {
    case SAND_WORM_WAIT:
        if (player_in_range(record->middle, WORM_NEAR) != 0) {
            record->made = 0;
            record->wait = 0;
            record->steps = 0;
            record->mode = SAND_WORM_LEAP;
        }
        break;
    case SAND_WORM_LEAP:
    default:
        if (player_in_range(record->middle, WORM_FAR) == 0) {
            task_destroy_children(task);
            record->mode = SAND_WORM_WAIT;
            break;
        }
        /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
        made = record->made;
        last = record->segments - 1;
        if (made > last) {
            if (task->child == NULL) {
                record->made = 0;
                record->wait = 0;
            }
        } else if (record->wait > 0) {
            record->wait--;
        } else {
            gaps = record->gaps;
            if (made == 0) {
                object = &sand_worm_head_object;
                record->wait = gaps[0];
            } else if (made < last) {
                object = &sand_worm_body_object;
                record->wait = made + 1 < last ? gaps[1] : gaps[2];
            } else if (made == last) {
                object = &sand_worm_tail_object;
            }
            task_create_desc_child(task, &sand_worm_segment_desc,
                                   (void *)&object);
            record->made++;
        }
        if (task->child == NULL) {
            record->steps = 0;
        } else {
            sand_worm_breach(task);
            record->steps++;
            rumble_near_camera(record->middle, RUMBLE_REACH);
            task_step_children(task);
        }
        break;
    }
}

static void sand_worm_destroy(struct task *task)
{
    struct sand_worm_record *record = (struct sand_worm_record *)task->motion;

    record->path->worms_left++;
}

/* start a segment at points[first], drawing *(NJS_OBJECT **)arg. with no
 * world object to lend, the segment destroys itself on its next step */
static void sand_worm_segment_init(struct task *task, void *arg)
{
    struct sand_worm_segment *segment;
    NJS_OBJECT *object;
    NJS_OBJECT *world;
    struct task_work *work;
    struct sand_worm_record *record;
    Float scale;

    object = *(NJS_OBJECT **)arg;
    segment = (struct sand_worm_segment *)task->motion;
    world = object_world_copy(object);
    segment->world = world;
    /* @hack redundant: the else after the return, which spills work through R3
     * before the 0 for segment->mode takes R3 (MOV.L R3,@R15 at
     * 0x0c904e02); without it, the 0 takes R3 first, work goes through R2,
     * and 3 of 72 words differ */
    if (world == NULL) {
        task_kill(task);
        return;
    } else {
        work = task->work;
        record = (struct sand_worm_record *)task->parent->motion;
        segment->mode = 0;
        segment->point = record->first;
        segment->reach = world->model->r * 2.0F;
        segment->object = object;
        sand_worm_segment_follow(task);
        object_force_clear(task->force);
        scale = record->scale;
        object_world_place(world, work, scale);
        object_world_add_sized(task, world, WORLD_MOVES | WORLD_SOLID, scale);
    }
}

/* draw the segment, place and test its world object, and move it a point
 * along the path. a segment already at points[last] destroys itself on its
 * next step instead */
static void sand_worm_segment_step(struct task *task)
{
    struct task_work *work = task->work;
    struct sand_worm_segment *segment =
        (struct sand_worm_segment *)task->motion;
    struct sand_worm_record *record =
        (struct sand_worm_record *)task->parent->motion;
    Float scale = record->scale;

    sand_worm_segment_draw(task);
    object_world_place(segment->world, work, scale);
    object_world_test_radius(work, segment->reach * scale);
    if (segment->point < record->last) {
        segment->point++;
        sand_worm_segment_follow(task);
    } else {
        task_kill(task);
    }
}

/* on a frame's first step, draw the segment at the path's scale, with the
 * stage's textures */
static void sand_worm_segment_draw(struct task *task)
{
    struct sand_worm_segment *segment;
    struct task_work *work;
    struct sand_worm_record *record;
    /* @hack redundant: ang register, which puts it in R14 and work in R13
     * (MOV R13,R14 at 0x0c904ed6); without it, the two trade, and 6 of 69
     * words differ */
    register struct angles *ang;
    Angle turn;
    Float scale;

    if (frame_step != 0) {
        return;
    }
    work = task->work;
    segment = (struct sand_worm_segment *)task->motion;
    record = (struct sand_worm_record *)task->parent->motion;
    scale = record->scale;
    ang = &work->ang;
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, ang->y, turn);
    TURN_VIA(njRotateX, ang->x, turn);
    njScale(NULL, scale, scale, scale);
    texture_set_stage();
    draw_tree(segment->object);
    njPopMatrix(1);
}

static void sand_worm_segment_destroy(struct task *task)
{
    struct sand_worm_segment *segment =
        (struct sand_worm_segment *)task->motion;

    object_world_free(task, segment->world);
}

/* move the segment to its point on the path, and face it along the path.
 * the force gets the move and the turn */
static void sand_worm_segment_follow(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    struct sand_worm_segment *segment =
        (struct sand_worm_segment *)task->motion;
    struct sand_worm_record *record =
        (struct sand_worm_record *)task->parent->motion;
    int index = segment->point;
    const NJS_POINT3 *point;

    point = &record->points[index];
    sand_worm_segment_move(&work->pos, &force->speed, point);
    sand_worm_segment_turn(&work->ang, &force->turn, point, index,
                           record->count);
}

/* move *pos to point, and set *speed to the move. speed->y stays 0 or
 * more, so a rider never moves down with the segment */
static void sand_worm_segment_move(NJS_POINT3 *pos, NJS_VECTOR *speed,
                                   const NJS_POINT3 *point)
{
    Float x = point->x;
    Float y = point->y;
    Float z = point->z;

    speed->x = x - pos->x;
    speed->y = y - pos->y;
    if (speed->y < 0.0F) {
        speed->y = 0.0F;
    }
    speed->z = z - pos->z;
    pos->x = x;
    pos->y = y;
    pos->z = z;
}

/* face ang along the path at point, which is points[index] of a path of
 * count points. ang->x and ang->y take the pitch and heading from the
 * point before to the point after. at an end, they take those between the
 * point and its neighbor. ang->z takes 0. turn gets how much each of ang's
 * angles changed */
static void sand_worm_segment_turn(struct angles *ang, struct angles *turn,
                                   const NJS_POINT3 *point, int index,
                                   int count)
{
    const NJS_POINT3 *from;
    const NJS_POINT3 *to;
    Float x;
    Float y;
    Float z;
    Angle pitch;
    Angle yaw;
    Angle roll;

    if (index == 0) {
        from = point;
        to = point + 1;
    } else if (index < count - 1) {
        from = point - 1;
        to = point + 1;
    } else {
        from = point - 1;
        to = point;
    }
    x = to->x - from->x;
    y = to->y - from->y;
    z = to->z - from->z;
    pitch = -(Angle)(atan2f(y, njSqrt((x * x) + (z * z))) * ANGLE_PER_RADIAN);
    yaw = (Angle)(atan2f(x, z) * ANGLE_PER_RADIAN);
    /* @hack spelling: the roll through a local, which SHC keeps as 0 in R5
     * for both uses (MOV #0,R5 at 0x0c9050d8); as 0 at each use, SHC
     * negates ang->z instead, and 26 of 89 words differ */
    roll = 0;
    turn->x = pitch - ang->x;
    turn->y = yaw - ang->y;
    turn->z = roll - ang->z;
    ang->x = pitch;
    ang->y = yaw;
    ang->z = roll;
}

/* on a breach's start step, clear its done and raise its smoke with
 * SE_SAND_WORM_BREACH. on its finish step, set done */
static void sand_worm_breach(struct task *task)
{
    int i = 0;
    struct sand_worm_record *record = (struct sand_worm_record *)task->motion;
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    int count = record->breach_count;
    int steps = record->steps;
    struct sand_worm_breach *breach = record->breaches;
    struct sand_smoke_arg smoke;
    struct task *child;

    for (; i < count; i++, breach++) {
        if (steps == breach->finish) {
            breach->done = TRUE;
        } else if (steps == breach->start) {
            breach->done = FALSE;
            smoke.pos = &record->points[breach->point];
            smoke.ang = &sand_worm_smoke_ang;
            smoke.shape = &breach->shape;
            smoke.done = &breach->done;
            child = task_create_desc_child(task, &sand_smoke_desc, &smoke);
            if (child != NULL) {
                sound_play_near(SE_SAND_WORM_BREACH, &child->work->pos,
                                &sand_worm_falloff);
            }
        }
    }
}
