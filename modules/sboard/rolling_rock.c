/* the rolling rocks, which roll down three routes. each route's watcher
 * makes its rock once a player comes near the route's foot. the first rock
 * rolls down from the top, and each later one rests at the foot */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/world.h"
#include "draw/tree.h"
#include "object/simple.h"
#include "player/player.h"
#include "sboard/act.h"
#include "sboard/rolling_rock.h"
#include "sboard/rumble.h"
#include "sound/cue.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/task_desc.h"
#include "system/texture.h"
#include "system/util.h"

/* a rolling rock's route. the rock starts at points[first], and rests at
 * the route's foot, points[count - 1 - trim]. watchers_left is how many more
 * watchers the route may take. a watcher's init takes one, and its destroy
 * gives it back. a watcher made with none left destroys itself on its next
 * step */
struct rolling_rock_route {
    Sint8 watchers_left;
    Sint8 first;
    Sint8 trim;
    Angle start_angle; /* the rock's ang.x at points[first] */
    Angle end_angle; /* the ang.x it comes to rest at */
    Angle step; /* how far ang.x turns each step */
    const NJS_POINT3 *points;
    int count;
    Float scale; /* the rock's size */
};

/* a watcher's record, in task->motion. init copies the route's fields here,
 * with last the index of the route's foot */
struct rolling_rock_watch {
    Sint8 mode;
    Sint16 first;
    Sint16 last;
    Angle start_angle;
    Angle end_angle;
    Angle step;
    const NJS_POINT3 *points;
    int count;
    Float scale;
    struct rolling_rock_route *route;
    const NJS_POINT3 *foot; /* points[last] */
};

/* a watcher's modes. WATCH_WAIT makes a rolling rock once a player is
 * within WATCH_NEAR of the foot. WATCH_AGAIN makes a resting one the same
 * way. WATCH_RUN runs the rock, and destroys it once no player is within
 * WATCH_FAR. a rock that destroys itself sends the watcher back to
 * WATCH_WAIT */
enum rolling_rock_watch_mode { WATCH_WAIT, WATCH_RUN, WATCH_AGAIN };

/* a rock's record, in task->motion */
struct rolling_rock_record {
    Sint8 mode;
    Sint16 point; /* the index of the route point it is at */
    Float reach; /* the world test's radius at scale 1: twice the model's */
    NJS_OBJECT *object;
    NJS_OBJECT *world; /* the rock's world object */
};

/* a rock's modes. its init takes a pointer to one as arg. ROCK_ROLL starts
 * the rock rolling at points[first], and ROCK_REST starts it at rest at the
 * foot */
enum rolling_rock_mode { ROCK_ROLL, ROCK_REST };

/* a player within WATCH_NEAR of a route's foot has its watcher make the
 * rock. the watcher destroys the rock once no player is within WATCH_FAR */
#define WATCH_NEAR 2000.0F
#define WATCH_FAR 2500.0F

/* a rolling rock rumbles the pad while it is within RUMBLE_REACH of the
 * camera */
#define RUMBLE_REACH 500.0F

#define SE_ROLLING_ROCK 696

extern NJS_OBJECT sand_rocky_head_object;

static void rolling_rock_watch_init(struct task *task, void *arg);
static void rolling_rock_watch_step(struct task *task);
static void rolling_rock_watch_destroy(struct task *task);
static void rolling_rock_init(struct task *task, void *arg);
static void rolling_rock_step(struct task *task);
static void rolling_rock_draw(struct task *task);
static void rolling_rock_destroy(struct task *task);
static void rolling_rock_follow(struct task *task);
static void rolling_rock_move(NJS_POINT3 *pos, NJS_VECTOR *speed,
                              const NJS_POINT3 *point);
static void rolling_rock_turn(struct angles *ang, struct angles *turn,
                              const NJS_POINT3 *point, int index, int count);

static const NJS_POINT3 rolling_rock_a_points[200] = {
#include "sboard/rolling_rock/rolling_rock_a_points.inc"
};

static const NJS_POINT3 rolling_rock_b_points[200] = {
#include "sboard/rolling_rock/rolling_rock_b_points.inc"
};

static const NJS_POINT3 rolling_rock_c_points[200] = {
#include "sboard/rolling_rock/rolling_rock_c_points.inc"
};

static const struct task_desc rolling_rock_watch_desc = {
    sizeof(struct rolling_rock_watch), 0,    rolling_rock_watch_init,
    rolling_rock_watch_step,           NULL, rolling_rock_watch_destroy};
static const struct task_desc rolling_rock_desc = {
    sizeof(struct rolling_rock_record),
    TASK_WORK | TASK_FORCE,
    rolling_rock_init,
    rolling_rock_step,
    rolling_rock_draw,
    rolling_rock_destroy};
static const struct sound_falloff rolling_rock_falloff = {127, -128, 100.0F,
                                                          2400.0F};

/* each route takes one watcher at a time. its rock starts at an ang.x of 80
 * degrees, and comes to rest at 0. it turns 5 degrees a step */
/* clang-format off */
static struct rolling_rock_route rolling_rock_a = {
    1, 0, 0, NJM_DEG_ANG(80), 0, NJM_DEG_ANG(5),
    rolling_rock_a_points, 200, 1.0F};
static struct rolling_rock_route rolling_rock_b = {
    1, 0, 0, NJM_DEG_ANG(80), 0, NJM_DEG_ANG(5),
    rolling_rock_b_points, 200, 1.0F};
static struct rolling_rock_route rolling_rock_c = {
    1, 0, 0, NJM_DEG_ANG(80), 0, NJM_DEG_ANG(5),
    rolling_rock_c_points, 200, 1.0F};
/* clang-format on */
/* the modes a watcher passes its rock's init */
static Sint8 rolling_rock_roll = ROCK_ROLL;
static Sint8 rolling_rock_rest = ROCK_REST;

struct task *rolling_rock_a_create(void)
{
    /* @hack spelling: the result through task, which keeps the call a JSR
     * (0x0c903a08); returned directly, it becomes a tail jump to
     * act_task_create, and 9 of 9 words differ */
    struct task *task =
        act_task_create(&rolling_rock_watch_desc, &rolling_rock_a);

    return task;
}

struct task *rolling_rock_b_create(void)
{
    /* @hack spelling: the result through task, which keeps the call a JSR
     * (0x0c903a1a); returned directly, it becomes a tail jump to
     * act_task_create, and 9 of 9 words differ */
    struct task *task =
        act_task_create(&rolling_rock_watch_desc, &rolling_rock_b);

    return task;
}

struct task *rolling_rock_c_create(void)
{
    /* @hack spelling: the result through task, which keeps the call a JSR
     * (0x0c903a2c); returned directly, it becomes a tail jump to
     * act_task_create, and 9 of 9 words differ */
    struct task *task =
        act_task_create(&rolling_rock_watch_desc, &rolling_rock_c);

    return task;
}

static void rolling_rock_watch_init(struct task *task, void *arg)
{
    struct rolling_rock_watch *watch =
        (struct rolling_rock_watch *)task->motion;
    struct rolling_rock_route *route = arg;
    int first;
    int end;
    int last;
    /* @hack redundant: points register, which keeps it in R4 and last in R7
     * (MOV.L @(16,R5),R4 at 0x0c903a60); without it, the two trade, and 13
     * of 52 words differ */
    register const NJS_POINT3 *points;

    watch->route = route;
    watch->route->watchers_left--;
    if (route->watchers_left < 0) {
        task_kill(task);
        return;
    }
    end = route->count;
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    first = route->first;
    /* @hack spelling: end and last set in one comma expression, which keeps
     * first on the stack (MOV.L R0,@R15 at 0x0c903a56); as two statements,
     * first takes R7 and last R1, the function runs 4 bytes short, and 50 of
     * 52 words differ */
    end -= route->trim, last = end - 1;
    points = route->points;
    watch->mode = WATCH_WAIT;
    watch->first = first;
    watch->last = last;
    watch->start_angle = (Uint16)route->start_angle;
    watch->end_angle = (Uint16)route->end_angle;
    watch->step = (Uint16)route->step;
    watch->points = points;
    watch->count = route->count;
    watch->scale = route->scale;
    watch->route = route;
    watch->foot = &points[last];
}

static void rolling_rock_watch_step(struct task *task)
{
    struct rolling_rock_watch *watch =
        (struct rolling_rock_watch *)task->motion;

    switch (watch->mode) {
    case WATCH_WAIT:
        if (player_in_range(watch->foot, WATCH_NEAR) != 0) {
            task_create_desc_child(task, &rolling_rock_desc,
                                   &rolling_rock_roll);
            watch->mode = WATCH_RUN;
        }
        break;
    case WATCH_RUN:
    default:
        if (player_in_range(watch->foot, WATCH_FAR) == 0) {
            task_destroy_children(task);
            watch->mode = WATCH_AGAIN;
        } else {
            task_step_children(task);
            if (task->child == NULL) {
                watch->mode = WATCH_WAIT;
            }
        }
        break;
    case WATCH_AGAIN:
        if (player_in_range(watch->foot, WATCH_NEAR) != 0) {
            task_create_desc_child(task, &rolling_rock_desc,
                                   &rolling_rock_rest);
            watch->mode = WATCH_RUN;
        }
        break;
    }
}

static void rolling_rock_watch_destroy(struct task *task)
{
    struct rolling_rock_watch *watch =
        (struct rolling_rock_watch *)task->motion;

    watch->route->watchers_left++;
}

/* start the rock rolling at points[first], or at rest at the foot, as the
 * mode at arg says. with no world object to lend, the rock destroys itself
 * on its next step */
static void rolling_rock_init(struct task *task, void *arg)
{
    NJS_OBJECT *object = &sand_rocky_head_object;
    struct rolling_rock_record *record =
        (struct rolling_rock_record *)task->motion;
    NJS_OBJECT *world = object_world_copy(object);
    struct rolling_rock_watch *watch;
    struct task_work *work;
    Float scale;
    Sint8 mode;

    record->world = world;
    if (world == NULL) {
        task_kill(task);
        return;
    }
    watch = (struct rolling_rock_watch *)task->parent->motion;
    work = task->work;
    mode = *(Sint8 *)arg;
    record->mode = mode;
    record->reach = world->model->r * 2.0F;
    record->object = object;
    switch (mode) {
    case ROCK_ROLL:
        record->point = watch->first;
        work->ang.x = watch->start_angle;
        break;
    case ROCK_REST:
    default:
        record->point = watch->last;
        work->ang.x = watch->end_angle;
        break;
    }
    rolling_rock_follow(task);
    object_force_clear(task->force);
    scale = watch->scale;
    object_world_place(world, work, scale);
    object_world_add_sized(task, world, WORLD_MOVES | WORLD_SOLID, scale);
}

/* a rolling rock moves a point a step to the foot. it then turns on until
 * its ang.x comes round to end_angle, and rests. while rolling, it plays
 * SE_ROLLING_ROCK and rumbles the pad. at rest, its force is zero */
static void rolling_rock_step(struct task *task)
{
    struct task_work *work = task->work;
    struct rolling_rock_record *record =
        (struct rolling_rock_record *)task->motion;
    struct task_force *force = task->force;
    struct rolling_rock_watch *watch =
        (struct rolling_rock_watch *)task->parent->motion;
    Float scale = watch->scale;
    Angle ang;
    Angle step;
    Angle end;

    rolling_rock_draw(task);
    object_world_place(record->world, work, scale);
    object_world_test_radius(work, record->reach * scale);
    switch (record->mode) {
    case ROCK_ROLL:
        ang = (Uint16)work->ang.x;
        step = watch->step;
        if (record->point < watch->last) {
            record->point++;
            ang += step;
        } else {
            end = watch->end_angle;
            if (ang > end) {
                end += 0x10000;
            }
            ang += step;
            if (ang >= end) {
                ang = end;
                record->mode = ROCK_REST;
            }
        }
        sound_keep_near(SE_ROLLING_ROCK, task, &work->pos,
                        &rolling_rock_falloff);
        rumble_near_camera(&work->pos, RUMBLE_REACH);
        rolling_rock_follow(task);
        force->turn.x = ang - work->ang.x;
        work->ang.x = ang;
        break;
    case ROCK_REST:
    default:
        object_force_clear((struct task_motion *)force);
        break;
    }
}

/* on a frame's first step, draw the rock at the route's scale, with the
 * stage's textures */
static void rolling_rock_draw(struct task *task)
{
    struct rolling_rock_record *record;
    struct task_work *work;
    struct rolling_rock_watch *watch;
    /* @hack redundant: ang register, which puts it in R14 and work in R13
     * (MOV R13,R14 at 0x0c903dbc); without it, the two trade, and 6 of 69
     * words differ */
    register struct angles *ang;
    Angle turn;
    Float scale;

    if (frame_step != 0) {
        return;
    }
    work = task->work;
    record = (struct rolling_rock_record *)task->motion;
    watch = (struct rolling_rock_watch *)task->parent->motion;
    scale = watch->scale;
    ang = &work->ang;
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, ang->y, turn);
    TURN_VIA(njRotateX, ang->x, turn);
    njScale(NULL, scale, scale, scale);
    texture_set_stage();
    draw_tree(record->object);
    njPopMatrix(1);
}

static void rolling_rock_destroy(struct task *task)
{
    struct rolling_rock_record *record =
        (struct rolling_rock_record *)task->motion;

    object_world_free(task, record->world);
}

/* move the rock to its point on the route, and face it along the route.
 * the force gets the move and the turn */
static void rolling_rock_follow(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    struct rolling_rock_record *record =
        (struct rolling_rock_record *)task->motion;
    struct rolling_rock_watch *watch =
        (struct rolling_rock_watch *)task->parent->motion;
    int index = record->point;
    const NJS_POINT3 *point;

    point = &watch->points[index];
    rolling_rock_move(&work->pos, &force->speed, point);
    rolling_rock_turn(&work->ang, &force->turn, point, index, watch->count);
}

/* move *pos to point, and set *speed to the move. speed->y stays 0 or
 * more, so a rider never moves down with the rock */
static void rolling_rock_move(NJS_POINT3 *pos, NJS_VECTOR *speed,
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

/* face ang along the route at point, which is points[index] of a route of
 * count points. ang->y takes the heading from the point before to the
 * point after. at an end, it takes the heading between the point and its
 * neighbor. ang->z takes 0. turn->y and turn->z get how much ang->y and
 * ang->z changed */
static void rolling_rock_turn(struct angles *ang, struct angles *turn,
                              const NJS_POINT3 *point, int index, int count)
{
    const NJS_POINT3 *from;
    const NJS_POINT3 *to;
    Angle yaw;
    Angle bank;
    Float dx;
    Float dz;

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
    dx = to->x - from->x;
    dz = to->z - from->z;
    yaw = (Angle)(atan2f(dx, dz) * ANGLE_PER_RADIAN);
    /* @hack spelling: the bank through a local, which SHC keeps as 0 in R5
     * for both stores (MOV #0,R5 at 0x0c903f5c); as 0 at each use, SHC
     * negates ang->z instead, and 21 of 53 words differ */
    bank = 0;
    turn->y = yaw - ang->y;
    turn->z = bank - ang->z;
    ang->y = yaw;
    ang->z = bank;
}
