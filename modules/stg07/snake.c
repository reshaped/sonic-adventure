/* the snake: the steps of two of the ruin's paths. ruin_path_snake_step
 * swims a snake of 44 segments round a closed curve near its path's points.
 * the snake rises and sinks with the water. no path list holds the other
 * step's path */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "collision/world.h"
#include "draw/model.h"
#include "path/curve.h"
#include "path/path.h"
#include "player/npc.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "stg07/snake.h"
#include "stg07/water_surface.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT snake_floor_object;
extern NJS_OBJECT snake_head_object;
extern NJS_OBJECT snake_joint_object;
extern NJS_OBJECT snake_body_object;
extern NJS_OBJECT snake_tail_object;
extern NJS_OBJECT snake_body_5_object;
extern NJS_OBJECT snake_narrow_body_object;

/* a path task's vars, laid out as path_vars. head, the task of the snake's
 * head segment, lies where path_vars has wait */
struct snake_vars {
    struct task *head;
    Sint16 stage_act;
    struct path *path;
};

#define VARS(work) ((struct snake_vars *)&(work)->vars)

/* in the snake's scale. SNAKE_COUNT is how many samples the curve has, and
 * SNAKE_START_Y the first sample's y. the snake's pos.y is how far it has
 * risen */
#define SNAKE_COUNT(work) ((work)->scl.z)
#define SNAKE_START_Y(work) ((work)->scl.x)

/* a segment's vars. the head keeps a floor in the world, which rises with
 * the snake. the world tests the floor only while a segment is touched.
 * the floor carries what stands on it by the head's force */
struct segment_vars {
    /* its spec's var: snake_floor_object's address for the head, which then
     * makes the floor, and 0 for the rest */
    Sint32 floor_object;
    Uint8 unk_04[4];
    NJS_OBJECT *floor; /* the head's floor in the world */
};

#define SEGMENT(work) ((struct segment_vars *)&(work)->vars)

/* a segment's kind, in its tick, from its spec's flags */
#define KIND(work) ((work)->tick)
/* how many samples along the curve it is */
#define ALONG(work) ((work)->scl.z)

/* snake_segments makes no KIND_JOINT_4 or KIND_NARROWER_JOINT */
enum segment_kind {
    KIND_HEAD,
    KIND_JOINT,
    KIND_BODY,
    KIND_TAIL,
    KIND_JOINT_4,
    KIND_BODY_5,
    KIND_NARROW_BODY,
    KIND_NARROW_JOINT,
    KIND_NARROWER_JOINT
};

enum snake_mode { SNAKE_START, SNAKE_SPAWN, SNAKE_SWIM, SNAKE_DONE };

enum segment_mode { SEGMENT_START = 0, SEGMENT_SWIM = 2, SEGMENT_DONE = 3 };

#define SE_GRIND 196

/* while the water is within SNAKE_SLACK of the snake's own level, the snake
 * stays. otherwise it climbs or sinks SNAKE_CLIMB a step toward it */
#define SNAKE_SLACK 10.0F
#define SNAKE_CLIMB 0.8F

/* the middle of the snake's room */
#define ROOM_X 5207.0F
#define ROOM_Y (-2557.0F)
#define ROOM_Z 1197.0F
/* the segments move only while player 0 is within SNAKE_NEAR of the room's
 * middle */
#define SNAKE_NEAR 800.0F

/* a segment's collision radius, which leaves room for its shape to rise
 * with the snake */
#define SEGMENT_REACH 600.0F

/* the head's floor stands at FLOOR_X, FLOOR_Y, FLOOR_Z before the snake
 * rises. it moves, so its mesh is made again each step */
#define FLOOR_X 5533.0F
#define FLOOR_Y (-2798.0F)
#define FLOOR_Z 1341.0F
#define FLOOR_FLAGS (WORLD_SMALL | WORLD_MOVES | WORLD_SOLID)

/* how far the snake rose this step, negative as it sinks. no code reads
 * it */
static Float snake_climb = 0.0F;
/* 1 once a touched segment has given the head its force this step */
static int snake_carried = 0;
/* no word in the image or in 1ST_READ.BIN points at these */
static Uint8 dat_0ca58300[12] = {0};

static void snake_segment_step(struct task *task);

/* a segment's spec. place goes in the spec's pos.y. the segment starts
 * (SEGMENT_LEAD - place) * SEGMENT_SPACING samples along the curve. the
 * places step by 0.75 from the head's 0, and the tail's by 1.5 */
#define SEGMENT_LEAD 39.0F
#define SEGMENT_SPACING 12.5F
/* clang-format off */
#define SEGMENT_SPEC(kind, place, var)                                         \
    {snake_segment_step, TASK_WORK | TASK_FORCE, (kind), {0, 0, 0},            \
     {0.0F, (place), 0.0F}, (var)}
/* clang-format on */

static struct task_spec snake_segments[] = {
    SEGMENT_SPEC(KIND_HEAD, 0.0F, (Sint32)&snake_floor_object),
    SEGMENT_SPEC(KIND_JOINT, 0.75F, 0),
    SEGMENT_SPEC(KIND_BODY, 1.5F, 0),
    SEGMENT_SPEC(KIND_JOINT, 2.25F, 0),
    SEGMENT_SPEC(KIND_BODY, 3.0F, 0),
    SEGMENT_SPEC(KIND_JOINT, 3.75F, 0),
    SEGMENT_SPEC(KIND_BODY, 4.5F, 0),
    SEGMENT_SPEC(KIND_JOINT, 5.25F, 0),
    SEGMENT_SPEC(KIND_BODY, 6.0F, 0),
    SEGMENT_SPEC(KIND_JOINT, 6.75F, 0),
    SEGMENT_SPEC(KIND_BODY, 7.5F, 0),
    SEGMENT_SPEC(KIND_JOINT, 8.25F, 0),
    SEGMENT_SPEC(KIND_BODY, 9.0F, 0),
    SEGMENT_SPEC(KIND_JOINT, 9.75F, 0),
    SEGMENT_SPEC(KIND_BODY, 10.5F, 0),
    SEGMENT_SPEC(KIND_JOINT, 11.25F, 0),
    SEGMENT_SPEC(KIND_BODY, 12.0F, 0),
    SEGMENT_SPEC(KIND_JOINT, 12.75F, 0),
    SEGMENT_SPEC(KIND_BODY, 13.5F, 0),
    SEGMENT_SPEC(KIND_JOINT, 14.25F, 0),
    SEGMENT_SPEC(KIND_BODY, 15.0F, 0),
    SEGMENT_SPEC(KIND_JOINT, 15.75F, 0),
    SEGMENT_SPEC(KIND_BODY, 16.5F, 0),
    SEGMENT_SPEC(KIND_JOINT, 17.25F, 0),
    SEGMENT_SPEC(KIND_BODY, 18.0F, 0),
    SEGMENT_SPEC(KIND_JOINT, 18.75F, 0),
    SEGMENT_SPEC(KIND_BODY, 19.5F, 0),
    SEGMENT_SPEC(KIND_JOINT, 20.25F, 0),
    SEGMENT_SPEC(KIND_BODY, 21.0F, 0),
    SEGMENT_SPEC(KIND_JOINT, 21.75F, 0),
    SEGMENT_SPEC(KIND_BODY, 22.5F, 0),
    SEGMENT_SPEC(KIND_JOINT, 23.25F, 0),
    SEGMENT_SPEC(KIND_BODY, 24.0F, 0),
    SEGMENT_SPEC(KIND_JOINT, 24.75F, 0),
    SEGMENT_SPEC(KIND_BODY, 25.5F, 0),
    SEGMENT_SPEC(KIND_JOINT, 26.25F, 0),
    SEGMENT_SPEC(KIND_BODY, 27.0F, 0),
    SEGMENT_SPEC(KIND_JOINT, 27.75F, 0),
    SEGMENT_SPEC(KIND_BODY, 28.5F, 0),
    SEGMENT_SPEC(KIND_JOINT, 29.25F, 0),
    SEGMENT_SPEC(KIND_BODY_5, 30.0F, 0),
    SEGMENT_SPEC(KIND_NARROW_JOINT, 30.75F, 0),
    SEGMENT_SPEC(KIND_NARROW_BODY, 31.5F, 0),
    SEGMENT_SPEC(KIND_TAIL, 33.0F, 0),
    {NULL, 0, 0, {0}, {0.0F, 0.0F, 0.0F}, 0},
};

/* a body's shape, the head's and the tail's. a segment's step keeps each
 * SHAPE_HEIGHT above the snake's rise */
#define SHAPE_HEIGHT 18.0F
/* clang-format off */
static struct collision_shape snake_shapes[3] = {
    {SHAPE_SPHERE, 0, PUSH_FIRM(PUSH_ALL), {-15.0F, SHAPE_HEIGHT, 0.0F}, 35.0F,
     13.0F, 13.0F, {0, 0, 0}, 0, NULL},
    {SHAPE_SPHERE, 0, PUSH_FIRM(PUSH_ALL), {-15.0F, SHAPE_HEIGHT, 20.0F}, 40.0F,
     13.0F, 63.0F, {0, 0, 0}, 0, NULL},
    {SHAPE_SPHERE, 0, PUSH_FIRM(PUSH_ALL), {-15.0F, SHAPE_HEIGHT, 20.0F}, 60.0F,
     13.0F, 63.0F, {0, 0, 0}, 0, NULL},
};
/* clang-format on */

/* 1 once a segment is touched this step */
static Uint8 snake_touched = 0;

/* ruin_water_level's first value. no word in the image or in 1ST_READ.BIN
 * points at it */
static Float dat_0ca58a00 = -2796.0F;

/* the curve's samples. the path's 40 points are its keys. a step of 1/30 takes
 * 30 samples a key, 1200 in all. the array holds 2000 */
static struct path_point snake_samples[2000];

/* a request to path_curve_sample, which reads curve alone */
struct snake_curve {
    struct path_curve curve;
    Sint32 unk_18;
};

static void snake_destroy(struct task *task);
static void snake_segment_draw(struct task *task);
static void snake_segment_destroy(struct task *task);
static void snake_floor_create(struct task *task);
static void snake_segment_move(struct path_point *samples, Float along,
                               struct task *task, struct task_work *snake);
static void snake_floor_move(struct task *task);

/* no task runs it, since no path list holds its path. it would end its task
 * on the task's fourth step, or once the stage or act changes */
void fun_0c907520(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack redundant: unused, the snake's room, which SHC copies onto the
     * frame and nothing reads (0x0c907528 to 0x0c90753a). without it, the
     * frame loses 12 bytes, and 47 of 50 words differ */
    NJS_POINT3 unused = {ROOM_X, ROOM_Y, ROOM_Z};

    if (stage_get_stage_act() != VARS(work)->stage_act) {
        task_kill(task);
        return;
    }
    switch (work->mode) {
    case 0:
        work->mode = 2;
        break;
    case 2:
        work->mode = 3;
        break;
    case 3:
        task_kill(task);
        break;
    }
    /* @hack redundant: a return where the function returns anyway, which ends
     * case 2 on its store, a BRA and a NOP (0x0c90756e to 0x0c907572). without
     * it, 15 of 50 words differ */
    return; /* NOLINT(readability-redundant-control-flow) */
}

/* the snake's task, which ends once the stage or act changes. it samples
 * the curve, makes the segments, then rises and sinks with the water */
void ruin_path_snake_step(struct task *task)
{
    NJS_POINT3 room = {ROOM_X, ROOM_Y, ROOM_Z};
    static const struct snake_curve snake_curve = {
        {NULL, 40, "SSFFFF", 1.0F / 30, snake_samples, PATH_CURVE_BSPLINE},
        0,
    };
    struct snake_curve request;
    struct path *path;
    struct task_work *player;
    struct task_work *work;
    Float above;

    work = task->work;
    player = player_work[0];
    if (stage_get_stage_act() != VARS(work)->stage_act) {
        task_kill(task);
        return;
    }
    switch (work->mode) {
    case SNAKE_START:
        task->destructor = snake_destroy;
        work->pos.x = 0.0F;
        work->scl.y = 0.0F;
        path = VARS(work)->path;
        request = snake_curve;
        request.curve.keys = path->points;
        SNAKE_COUNT(work) = path_curve_sample((struct path_curve *)&request);
        SNAKE_START_Y(work) = snake_samples[0].pos.y;
        work->mode = SNAKE_SPAWN;
        break;
    case SNAKE_SPAWN:
        task_create_children(snake_segments, task);
        work->mode = SNAKE_SWIM;
        sound_play_held(SE_GRIND, (int)task, 1, 0);
        sound_set_volume((int)task, 90);
        break;
    case SNAKE_SWIM:
        if (ruin_water_level != 0.0F) {
            above = ruin_water_level - SNAKE_START_Y(work) - work->pos.y;
            if (above > SNAKE_SLACK) {
                work->pos.y += SNAKE_CLIMB;
                snake_climb = SNAKE_CLIMB;
            } else if (above < -SNAKE_SLACK) {
                work->pos.y -= SNAKE_CLIMB;
                snake_climb = -SNAKE_CLIMB;
            } else {
                snake_climb = 0.0F;
            }
        }
        if (njDistanceP2P(&player->pos, &room) < SNAKE_NEAR) {
            task_step_children(task);
            snake_carried = 0;
        }
        snake_touched = 0;
        break;
    case SNAKE_DONE:
        break;
    }
}

static void snake_destroy(struct task *task)
{
    /* @hack spelling: snake, a copy of task, which SHC keeps in R14 across
     * sound_stop_id (MOV R4,R14 at 0x0c90777a). with task alone, 10 of 10
     * words differ */
    struct task *snake = task;

    sound_stop_id((int)snake);
    task_destroy_children(snake);
}

/* a segment's step. its first step places the segment and gives it its
 * shape. each later step moves it a sample along the curve */
static void snake_segment_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task *snake = task->parent;
    struct task_work *snake_work = snake->work;

    switch (work->mode) {
    case SEGMENT_START:
        task->draw = snake_segment_draw;
        task->destructor = snake_segment_destroy;
        ALONG(work) = (SEGMENT_LEAD - work->pos.y) * SEGMENT_SPACING;
        KIND(work) = (Uint8)work->flags;
        if (KIND(work) == KIND_HEAD) {
            VARS(snake_work)->head = task;
        }
        work->mode = SEGMENT_SWIM;
        work->pos.x = snake_samples[0].pos.x;
        work->pos.y = snake_samples[0].pos.y;
        work->pos.z = snake_samples[0].pos.z;
        if (SEGMENT(work)->floor_object != 0 && KIND(work) == KIND_HEAD) {
            snake_floor_create(task);
        }
        if (KIND(work) == KIND_BODY || KIND(work) == KIND_BODY_5 ||
            KIND(work) == KIND_NARROW_BODY) {
            collision_init(task, &snake_shapes[0], COLLISION_OBJECT, 1, 1);
            work->collision->radius = SEGMENT_REACH;
        } else if (KIND(work) == KIND_HEAD) {
            collision_init(task, &snake_shapes[1], COLLISION_OBJECT, 1, 1);
            work->collision->radius = SEGMENT_REACH;
        } else if (KIND(work) == KIND_TAIL) {
            collision_init(task, &snake_shapes[2], COLLISION_OBJECT, 1, 1);
            work->collision->radius = SEGMENT_REACH;
        }
        break;
    case SEGMENT_SWIM:
        snake_segment_move(snake_samples, ALONG(work), task, snake_work);
        if (KIND(work) == KIND_HEAD) {
            snake_floor_move(task);
        }
        if (KIND(work) != KIND_JOINT) {
            if (KIND(work) == KIND_HEAD || KIND(work) == KIND_BODY ||
                KIND(work) == KIND_BODY_5 || KIND(work) == KIND_NARROW_BODY ||
                KIND(work) == KIND_TAIL) {
                /* @hack spelling: the shapes' address through a local, which
                 * SHC loads before the add (MOV.L @(12,R4),R4 at
                 * 0x0c90790e). stored through the record, 7 of 184 words
                 * differ */
                struct collision_shape *shape = work->collision->shapes;

                shape->center.y = snake->work->pos.y + SHAPE_HEIGHT;
            }
            collision_register(work);
        }
        ++work->timer;
        snake_segment_draw(task);
        break;
    case SEGMENT_DONE:
        break;
    }
    /* @hack redundant: a return where the function returns anyway, which sends
     * each collision case to one shared return (BRA at 0x0c907852 and
     * 0x0c907876). without it, 113 of 184 words differ */
    return; /* NOLINT(readability-redundant-control-flow) */
}

static void snake_segment_draw(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack redundant: unused, which SHC copies onto the frame and nothing
     * reads (0x0c907946 to 0x0c907958). without it, 131 of 140 words
     * differ */
    NJS_VECTOR unused = {0.0F, -1.0F, 0.0F};
    /* @hack redundant: more, which the draw never reads, makes its frame the
     * image's 28 bytes (ADD #-28,R15 at 0x0c907944). without it, 3 of 140
     * words differ */
    NJS_POINT3 more;
    struct task *snake;
    NJS_OBJECT *object;
    Angle ang;

    snake = task->parent;
    texture_set_stage();
#ifdef GCC
    /* GCC can't see that every path sets object: each segment's kind comes
     * from its spec, which gives one the switch has a case for */
    object = &snake_body_object;
#endif
    switch (KIND(work)) {
    case KIND_BODY:
        object = &snake_body_object;
        break;
    case KIND_JOINT:
    case KIND_JOINT_4:
    case KIND_NARROW_JOINT:
    case KIND_NARROWER_JOINT:
        object = &snake_joint_object;
        break;
    case KIND_HEAD:
        object = &snake_head_object;
        break;
    case KIND_TAIL:
        object = &snake_tail_object;
        break;
    case KIND_BODY_5:
        object = &snake_body_5_object;
        break;
    case KIND_NARROW_BODY:
        object = &snake_narrow_body_object;
        break;
    }
    if (TASK_TIMER(work) == 30) {
        work->timer = 0;
    }
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y + snake->work->pos.y,
                work->pos.z);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    if (KIND(work) == KIND_NARROW_JOINT) {
        njScale(NULL, 0.7F, 1.0F, 1.0F);
    } else if (KIND(work) == KIND_NARROWER_JOINT) {
        njScale(NULL, 0.5F, 0.8F, 1.0F);
    }
    draw_model(object->model);
    njPopMatrix(1);
}

/* as the head goes, it takes its floor out of the world, and Tails can be
 * seen again */
static void snake_segment_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (KIND(work) == KIND_HEAD) {
        world_remove(task, SEGMENT(work)->floor);
        world_object_free(SEGMENT(work)->floor);
        npc_set_disabled(FALSE);
    }
}

static void snake_floor_create(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *source = &snake_floor_object;
    NJS_OBJECT *floor = world_object_alloc();

    SEGMENT(work)->floor = floor;
    floor->scl[0] = 1.0F;
    floor->scl[1] = 1.0F;
    floor->scl[2] = 1.0F;
    floor->ang[0] = 0;
    floor->ang[1] = 0;
    floor->ang[2] = 0;
    floor->pos[0] = FLOOR_X;
    floor->pos[1] = FLOOR_Y;
    floor->pos[2] = FLOOR_Z;
    floor->model = source->model;
    floor->child = NULL;
    floor->sibling = NULL;
    world_add(FLOOR_FLAGS, task, floor);
}

/* move the segment to the sample at index along, and turn it as the curve
 * runs there. each segment's task_force takes its move and its turn, but
 * only the head's keeps them. the first head, body or tail touched in a step
 * gives the head its force. the head's floor carries what stands on it by
 * that force. snake is unused */
static void snake_segment_move(struct path_point *samples, Float along,
                               struct task *task, struct task_work *snake)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    struct task *parent = task->parent;
    struct task_work *parent_work = parent->work;
    struct task *head;
    /* @hack redundant: unused, which takes the sample's place and is never
     * read (stores at 0x0c907be0 to 0x0c907bee). without it, 316 of 339
     * words differ */
    NJS_POINT3 unused;
    NJS_VECTOR move;
    Angle ang[3];
    /* @hack redundant: more, which the move never reads, makes its frame the
     * image's 68 bytes (ADD #-68,R15 at 0x0c907bb4). without it, 7 of 339
     * words differ */
    NJS_POINT3 more;
    NJS_VECTOR ahead;

    unused.x = samples[(int)along].pos.x;
    unused.y = samples[(int)along].pos.y;
    unused.z = samples[(int)along].pos.z;
    if (!(along < SNAKE_COUNT(parent_work))) {
        move.x = samples[0].pos.x - work->pos.x;
        move.y = samples[0].pos.y - work->pos.y;
        move.z = samples[0].pos.z - work->pos.z;
    } else {
        move.x = samples[(int)along].pos.x - work->pos.x;
        move.y = samples[(int)along].pos.y - work->pos.y;
        move.z = samples[(int)along].pos.z - work->pos.z;
    }
    ang[0] = samples[(int)along].ang_x;
    ang[2] = samples[(int)along].ang_z;
    force->turn.x = angle_difference(work->ang.x, ang[0]);
    force->turn.z = angle_difference(work->ang.z, ang[2]);
    work->ang.x = ang[0];
    work->ang.z = ang[2];
    ALONG(work) += 1.0F;
    if (along + 2.0F < SNAKE_COUNT(parent_work)) {
        ahead.x = samples[(int)along + 2].pos.x - samples[(int)along].pos.x;
        ahead.y = samples[(int)along + 2].pos.y - samples[(int)along].pos.y;
        ahead.z = samples[(int)along + 2].pos.z - samples[(int)along].pos.z;
    } else if (along + 2.0F == SNAKE_COUNT(parent_work)) {
        ahead.x = samples[0].pos.x - samples[(int)along + 1].pos.x;
        ahead.y = samples[0].pos.y - samples[(int)along + 1].pos.y;
        ahead.z = samples[0].pos.z - samples[(int)along + 1].pos.z;
    } else {
        ALONG(work) = 0.0F;
        ahead.x = samples[1].pos.x - samples[(int)along].pos.x;
        ahead.y = samples[1].pos.y - samples[(int)along].pos.y;
        ahead.z = samples[1].pos.z - samples[(int)along].pos.z;
    }
    ang[1] = (Angle)(atan2f(ahead.x, ahead.z) * ANGLE_PER_RADIAN);
    force->turn.y = -angle_difference(work->ang.y, ang[1]);
    work->ang.y = ang[1];
    force->speed.x = move.x;
    force->speed.y = move.y;
    force->speed.z = move.z;
    work->pos.x += move.x;
    work->pos.y += move.y;
    work->pos.z += move.z;
    if (KIND(work) == KIND_BODY || KIND(work) == KIND_TAIL ||
        KIND(work) == KIND_HEAD || KIND(work) == KIND_BODY_5) {
        if (work->collision->flags & COLLISION_HIT) {
            work->collision->flags &= ~COLLISION_HIT;
            head = VARS(task->parent->work)->head;
            head->work->flags |= WORLD_TESTED;
            snake_touched = 1;
            if (snake_carried == 0) {
                ((struct task_force *)head->force)->speed.x = force->speed.x;
                ((struct task_force *)head->force)->speed.y = force->speed.y;
                ((struct task_force *)head->force)->speed.z = force->speed.z;
                ((struct task_force *)head->force)->turn.y = force->turn.y;
                ((struct task_force *)head->force)->turn.x = force->turn.x;
                ((struct task_force *)head->force)->turn.z = force->turn.z;
                snake_carried = 1;
            }
        } else {
            head = VARS(task->parent->work)->head;
            if (snake_touched != 1) {
                head->work->flags &= ~WORLD_TESTED;
            }
        }
    }
    head = VARS(task->parent->work)->head;
    if (head != task) {
        force->speed.x = 0.0F;
        force->speed.y = 0.0F;
        force->speed.z = 0.0F;
        force->turn.z = 0;
        force->turn.x = 0;
        force->turn.y = 0;
    }
}

/* the floor rises and sinks with the snake */
static void snake_floor_move(struct task *task)
{
    struct task *snake = task->parent;
    struct task_work *work = task->work;
    NJS_OBJECT *floor = SEGMENT(work)->floor;

    floor->pos[1] = snake->work->pos.y + FLOOR_Y;
}
