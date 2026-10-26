/* O LION, whose sides rise and slam down in turn, and whose middle sinks and
 * rises */
#include <ninja.h>
#include <sg_xpt.h>
#include <string.h>

#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "effect/dust.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg09/casino.h"
#include "stg09/helpers.h"
#include "stg09/lion.h"
#include "stg09/quake.h"
#include "system/task.h"
#include "system/util.h"

/* the model the lion draws. its root's children, nodes 0 to 4, are in turn
 * the middle, the right side, node 2, the left side and node 4. nodes 2 and
 * 4 don't move */
extern NJS_OBJECT lion_object;
/* the sides as the collision world holds them. its root's children are the
 * left side, then the right */
extern NJS_OBJECT lion_world_object;
/* meshsets 5 and 10 of node 2's model, whose UVs shift */
extern NJS_MESHSET lion_meshset_5;
extern NJS_MESHSET lion_meshset_10;

/* destroyed beyond about 800 units of the camera and player 0 */
#define RANGE ((800.0F * 800.0F) + 10.0F)
/* the radius of its copy in the collision world, and of the sphere its draw
 * tests */
#define REACH 320.0F

#define SE_MIDDLE_DOWN 276

/* the cycle starts only while player 0, player 1 or the camera is at most
 * START_BELOW below the lion. the collision world tests the parts only while
 * one is at most TEST_BELOW below it */
#define START_BELOW 20.0F
#define TEST_BELOW 80.0F

/* a puff of dust's speed out from the ring's center, and up */
#define DUST_OUT 3.0F
#define DUST_UP 1.0F

/* a slam pushes player 0 up by this share of the push, on top of the push */
#define PUSH_UP 0.5F

/* the modes, in the order it runs them */
#define LION_WAITING 0
#define LION_LEFT_RISING 1
#define LION_LEFT_SLAMMING 2
#define LION_RIGHT_RISING 3
#define LION_RIGHT_SLAMMING 4
#define LION_MIDDLE_SINKING 5
#define LION_MIDDLE_RISING 6

/* the shapes lion_run switches on and off. the HURT shapes hurt players, and
 * the BOX shapes are solid. the sides' shapes rise and fall with their
 * sides, and the middle's stays where it is. SHAPES counts every shape */
#define SHAPE_RIGHT_HURT 0
#define SHAPE_LEFT_HURT 1
#define SHAPE_MIDDLE_HURT 2
#define SHAPE_RIGHT_BOX 3
#define SHAPE_LEFT_BOX 4
#define SHAPES 16

/* the sine of phase times a quarter turn, which rises from 0 to 1 as phase
 * does */
#define QUARTER_SINE(phase) njSin((Angle)((phase) * (90.0F * ANGLE_PER_DEGREE)))

/* the copies of the lion's nodes, which lion_init allocates on the heap and
 * lion_destroy frees. middle, right, node_2 and left are siblings, in that
 * order. they are the children of the copy the collision world holds. node_4
 * is only drawn. each side copies its node in lion_world_object, and draws with
 * its node's model in lion_object */
struct lion_parts {
    /* how far each part stands above its place in the model: only y moves */
    union point_floats left_offset;
    union point_floats right_offset;
    union point_floats middle_offset;
    NJS_OBJECT left; /* at -x */
    NJS_MODEL *left_model;
    NJS_OBJECT right; /* at +x */
    NJS_MODEL *right_model;
    NJS_OBJECT middle;
    NJS_OBJECT node_4;
    NJS_OBJECT node_2;
    /* the root model of its copy in the collision world, zeroed but for its
     * radius */
    NJS_MODEL world_model;
    /* how far each side bounces above its offset after it slams, and how
     * fast */
    Float left_bounce;
    Float right_bounce;
    Float left_bounce_speed;
    Float right_bounce_speed;
};

struct lion_vars {
    NJS_OBJECT *world; /* its copy in the collision world */
    Uint8 unk_04[4];
    struct lion_parts *parts;
};

#define VARS(work) ((struct lion_vars *)&(work)->vars)
#define PARTS(work) (VARS(work)->parts)
/* the steps lion_run waits before it runs the mode again. the bounces run
 * on while it waits. WAIT is a Sint8, so lion_middle_down_steps' 150 is
 * stored as -106. it still takes 150 steps to count down to 0, through -128
 * and 127 */
#define WAIT(work) ((work)->submode)
/* counts from 0 to twice lion_shift_steps - 1, and round again. the UVs shift
 * while it's at lion_shift_steps or more */
#define SHIFT_STEP(work) ((work)->tick)

/* the steps a side takes to rise, and to slam down. it holds at the top for
 * lion_side_hold_steps between them */
static int lion_side_rise_steps = 90;
static int lion_side_slam_steps = 15;
static int lion_side_hold_steps = 15;
/* the steps the middle takes to sink, stays down, and takes to rise */
static int lion_middle_sink_steps = 30;
static int lion_middle_down_steps = 150;
static int lion_middle_rise_steps = 15;
/* the steps the lion rests after each slam, and after the middle rises */
static int lion_rest_steps = 10;
/* the cycle starts once the player nearest the lion is nearer than this */
static Float lion_start_distance = 400.0F;
/* how high a side rises, and how high the middle stands until it sinks */
static Float lion_side_lift = 30.0F;
static Float lion_middle_height = 60.0F;
/* the point a slam's dust, quake and push center on, in the lion's model
 * space. the right side's is at lion_slam_x, and the left's at -lion_slam_x */
static Float lion_slam_x = 120.0F;
static Float lion_slam_y = 5.0F;
static Float lion_slam_z = 210.0F;
static Float lion_dust_radius = 50.0F;
static int lion_dust_count = 18;
static Float lion_dust_scale = 7.0F;
/* the reach of a slam's quake and push */
static Float lion_slam_reach = 250.0F;
/* the push at the slam's point, falling to 0 at its reach */
static Float lion_slam_push = 3.0F;
/* each step, a side's bounce moves by its speed. the speed then keeps
 * lion_bounce_keep of itself, and loses lion_bounce_spring of the bounce. a
 * bounce that falls below 0 stops at 0, and its speed turns back. each slam
 * adds lion_bounce_kick to the side's speed */
static Float lion_bounce_keep = 0.91F;
static Float lion_bounce_spring = 0.75F;
static Float lion_bounce_kick = 3.0F;
static Float lion_quake_speed = 2.0F;
static int lion_shift_steps = 8;
/* the UVs' one shifted phase: meshsets 10 and 5 moved half the texture
 * along u */
static struct uv_shift lion_shifts[1][2] = {
#include "stg09/lion/lion_shifts.inc"
};
/* clang-format off */
static struct collision_shape lion_shapes[SHAPES] = {
    {SHAPE_CYLINDER | SHAPE_HURTABLE,
     SHAPE_HURTS_PLAYERS | SHAPE_GUARD | ATTACK(2),
     PUSH_ALL | PUSH_FIRM(PUSH_ALL), {120.0F, 0.0F, 220.0F}, 50.0F, 2.0F, 0.0F,
     {0}, 0, NULL},
    {SHAPE_CYLINDER | SHAPE_HURTABLE,
     SHAPE_HURTS_PLAYERS | SHAPE_GUARD | ATTACK(2),
     PUSH_ALL | PUSH_FIRM(PUSH_ALL), {-120.0F, 0.0F, 220.0F}, 50.0F, 2.0F, 0.0F,
     {0}, 0, NULL},
    {SHAPE_SPHERE | SHAPE_HURTABLE,
     SHAPE_HURTS_PLAYERS | SHAPE_GUARD | ATTACK(2),
     PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 125.0F, 150.0F}, 15.0F, 0.0F, 0.0F,
     {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {109.92F, 20.0F, 196.6F}, 54.0F, 20.0F, 100.0F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-116.08F, 20.0F, 196.6F}, 54.0F, 20.0F, 100.0F, {0}, 0, NULL},
    {SHAPE_CAPSULE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {2.6F, 150.0F, 13.75F}, 140.0F, 20.0F, 0.0F, {0x1000, 0, 0}, 0, NULL},
    {SHAPE_CAPSULE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-1.49F, 178.06F, 142.660004F}, 50.0F, 20.0F, 0.0F, {0x5000, 0, 0},
     0, NULL},
    {SHAPE_SPHERE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {112.0F, 0.0F, 117.67001F}, 80.0F, 0.0F, 0.0F, {0, 0x3b80, 0}, 0, NULL},
    {SHAPE_SPHERE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-112.0F, 0.0F, 117.67001F}, 80.0F, 0.0F, 0.0F, {0, 0x8cf0, 0}, 0, NULL},
    {SHAPE_CYLINDER2, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 41.99F, 37.6200104F}, 70.0F, 149.0F, 0.0F, {0, 0, 0x4000},
     0, NULL},
    {SHAPE_SPHERE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {155.0F, 86.0F, 52.01001F}, 60.0F, 0.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_SPHERE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-155.0F, 86.0F, 52.01001F}, 60.0F, 0.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {140.0F, -13.0F, 169.9F}, 105.0F, 10.0F, 125.0F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-140.0F, -13.0F, 169.9F}, 105.0F, 10.0F, 125.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER2, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {158.0F, 36.67F, 86.630005F}, 60.0F, 8.0F, 0.0F, {0, 0, 0x4000},
     0, NULL},
    {SHAPE_CYLINDER2, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-158.96F, 38.67F, 85.630005F}, 60.0F, 8.0F, 0.0F, {0, 0, 0x4000},
     0, NULL},
};
/* clang-format on */

static void lion_init(struct task *task);
static void lion_destroy(struct task *task);
static void lion_run(struct task *task);
static void lion_draw(struct task *task);

/* raise count puffs of dust of scale, in a ring of radius about point. point
 * is in model space, about the work's position and turned by its y angle */
static void lion_dust_ring(NJS_POINT3 *point, struct task_work *work,
                           Float radius, int count, Float scale)
{
    NJS_POINT3 pos;
    NJS_POINT3 center;
    NJS_VECTOR speed;
    Angle ang;
    Angle step;

    /* the step between puffs, in whole degrees */
    /* NOLINTNEXTLINE(bugprone-integer-division) */
    step = (360 / count) * ANGLE_PER_DEGREE;
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    njCalcPoint(NULL, point, &center);
    njPopMatrix(1);
    ang = work->ang.y;
    for (; count--; ang += step) {
        Float cosine;
        Float sine;
        Float up;

        cosine = njCos(ang);
        /* @hack spelling: the rise through the local up, set between the
         * calls. SHC then loads it into FR12 after njCos on each pass (FLDI1
         * at 0x0c91065c). as DUST_UP, SHC loads it once ahead of the loop,
         * and 6 of lion_dust_ring's 113 words differ */
        up = DUST_UP;
        sine = njSin(ang);
        pos.x = center.x + (radius * cosine);
        pos.y = center.y;
        pos.z = center.z + (radius * sine);
        speed.x = cosine * DUST_OUT;
        speed.y = up;
        speed.z = sine * DUST_OUT;
        dust_create(&pos, &speed, scale);
    }
}

/* raise the ring of dust where a side slams. right picks the right side, and
 * 0 the left */
static void lion_slam_dust(int right, struct task_work *work)
{
    union point_floats point;
    Float *at;
    /* @hack spelling: the slam's point through locals. SHC then loads the
     * three addresses ahead of the branch on right, x's into R3, y's into R6
     * and z's into R7 (0x0c910724 to 0x0c91072a). read where they're used,
     * 21 of lion_slam_dust's 31 words differ */
    Float *z = &lion_slam_z;
    Float *y = &lion_slam_y;
    Float x = lion_slam_x;

    /* @hack redundant: a test of y that never runs, a use of y that SHC counts;
     * without it, y's and z's addresses trade R6 and R7 and 4 of 31 words
     * differ */
    if (0) {
        if (y) {
        }
    }
    /* @hack spelling: the point's floats reached through &point, whose address
     * SHC forms from R15 in each arm (MOV R15,R4 at 0x0c910732 and
     * 0x0c910736). through point.elements, 30 of lion_slam_dust's 31 words
     * differ */
    /* @hack spelling: the cursor at set in each arm. set once ahead of the
     * branch, 28 of lion_slam_dust's 31 words differ */
    if (right) {
        at = (&point)->elements;
        *at = x;
    } else {
        at = (&point)->elements;
        *at = -x;
    }
    ++at;
    *at++ = *y;
    *at = *z;
    lion_dust_ring(&point.point, work, lion_dust_radius, lion_dust_count,
                   lion_dust_scale);
}

/* out = to - from, over three floats */
#pragma inline(lion_difference)
static void lion_difference(Float *from, Float *to, Float *out)
{
    *out++ = *to++ - *from++;
    *out++ = *to++ - *from++;
    *out = *to - *from;
}

/* play SE_SLAM for a side's slam. right picks the right side, and 0 the
 * left. then start a quake at the slam's point, and push player 0 away from
 * it */
static void lion_slam(int right, struct task_work *work)
{
    union point_floats point;
    Float *at;
    /* @hack spelling: the slam's point through locals, as in lion_slam_dust.
     * x's address takes R3, y's R5 and z's R6 (0x0c91076c to 0x0c910770).
     * read where they're used, 135 of lion_slam's 144 words differ */
    Float *z = &lion_slam_z;
    Float *y = &lion_slam_y;
    Float x = lion_slam_x;
    union point_floats away;
    Float distance;

    /* @hack redundant: a test of y that never runs, a use of y that SHC counts;
     * without it, y's and z's addresses trade R5 and R6 and 7 of 144 words
     * differ */
    if (0) {
        if (y) {
        }
    }
    /* @hack spelling: the point's floats reached through &point, whose address
     * SHC forms from R15 in each arm (MOV R15,R4 at 0x0c910776 and
     * 0x0c9107a0). through point.elements, the object builds 32 bytes shorter,
     * and 142 of lion_slam's 144 words differ */
    /* @hack spelling: the cursor at set in each arm. set once ahead of the
     * branch, the object builds 32 bytes shorter, and 142 of lion_slam's 144
     * words differ */
    if (right) {
        at = (&point)->elements;
        *at = x;
    } else {
        at = (&point)->elements;
        *at = -x;
    }
    ++at;
    *at++ = *y;
    *at = *z;
    sound_play(SE_SLAM, 0, 0, 0);
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    njCalcPoint(NULL, &point.point, &point.point);
    njPopMatrix(1);
    quake_start(&point.point, lion_slam_reach, lion_quake_speed);
    distance = njDistanceP2P(&player_work[0]->pos, &point.point);
    if (distance <= lion_slam_reach && player_work[0]->pos.y < point.point.y &&
        (player_work[0]->flags & PLAYER_ON_GROUND)) {
        /* @hack spelling: the difference through lion_difference, which SHC
         * expands here, and which loads the player's y ahead of the point's
         * (0x0c91084e, 0x0c910850). written out in place, the two trade, and
         * 2 of lion_slam's 144 words differ */
        /* @hack spelling: away's floats reached through &away, whose address
         * SHC forms from R15 at each use (MOV R15,R6 at 0x0c91083a, MOV R15,R4
         * at 0x0c91085c). through away.elements, the object builds 32 bytes
         * shorter, and 61 of lion_slam's 144 words differ */
        lion_difference(point.elements, VECTOR_FLOATS(&player_work[0]->pos),
                        (&away)->elements);
        njUnitVector(&away.vector);
        distance = (1.0F - (distance / lion_slam_reach)) * lion_slam_push;
        player_set_accel(0, away.vector.x * distance,
                         (away.vector.y + PUSH_UP) * distance,
                         away.vector.z * distance);
    }
}

void lion_step(struct task *task)
{
    /* @hack spelling: a copy of task, which keeps task in R14 across the
     * range check (MOV R4,R14 at 0x0c910928). without it, SHC keeps task on
     * the stack, the object builds 32 bytes longer, and 12 of lion_step's 16
     * words differ */
    struct task *current = task;

    if (!placement_try_leave_by(current, RANGE)) {
        lion_init(current);
    }
}

static void lion_init(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *root;
    NJS_OBJECT *world;
    NJS_OBJECT *node;
    NJS_OBJECT *sides;
    NJS_OBJECT *right;
    NJS_OBJECT copy;

    PARTS(work) = heap_alloc(sizeof(struct lion_parts));
    root = &lion_object;
    world = &lion_world_object;
    memset(&PARTS(work)->world_model, 0, sizeof(NJS_MODEL));
    PARTS(work)->world_model.r = REACH;
    node = root->child;
    sides = world->child;
    PARTS(work)->middle = *node;
    PARTS(work)->middle.evalflags &= ~NJD_EVAL_UNIT_POS;
    PARTS(work)->middle.sibling = &PARTS(work)->right;
    right = node->sibling;
    PARTS(work)->right = *sides->sibling;
    PARTS(work)->right_model = right->model;
    PARTS(work)->right.evalflags &= ~NJD_EVAL_UNIT_POS;
    PARTS(work)->right.sibling = &PARTS(work)->node_2;
    node = right->sibling;
    PARTS(work)->node_2 = *node;
    PARTS(work)->node_2.evalflags &= ~NJD_EVAL_UNIT_POS;
    PARTS(work)->node_2.sibling = &PARTS(work)->left;
    node = node->sibling;
    PARTS(work)->left = *sides;
    PARTS(work)->left_model = node->model;
    PARTS(work)->left.evalflags &= ~NJD_EVAL_UNIT_POS;
    PARTS(work)->left.sibling = NULL;
    node = node->sibling;
    PARTS(work)->node_4 = *node;
    PARTS(work)->node_4.evalflags &= ~NJD_EVAL_UNIT_POS;
    PARTS(work)->node_4.sibling = NULL;
    PARTS(work)->left_bounce = 0.0F;
    PARTS(work)->right_bounce = 0.0F;
    PARTS(work)->left_bounce_speed = 0.0F;
    PARTS(work)->right_bounce_speed = 0.0F;
    if (!placement_editing(task)) {
        work->ang.x = 0;
        work->ang.z = 0;
        copy = lion_object;
        copy.child = &PARTS(work)->middle;
        copy.model = &PARTS(work)->world_model;
        casino_world_add(task, &copy, REACH, WORLD_MOVES | WORLD_GRIP_HIGH);
        collision_init(task, lion_shapes, COLLISION_OBJECT, 1, SHAPES);
    }
    task->draw = lion_draw;
    task->step = lion_run;
    task->destructor = lion_destroy;
    work->timer = 0;
    WAIT(work) = 0;
    work->mode = LION_WAITING;
    SHIFT_STEP(work) = 0;
    uv_shift_make(lion_shifts[0], 2);
}

static void lion_destroy(struct task *task)
{
    struct task_work *work = task->work;

    casino_world_remove(task);
    heap_free(PARTS(work));
    uv_shift_free(lion_shifts[0], 2);
}

static void lion_run(struct task *task)
{
    struct task_work *work = task->work;
    struct player *player = player_get(0);
    NJS_VECTOR *force;
    Float *at;
    Float phase;
    Float height;
    int nearest;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    lion_draw(task);
    if (placement_editing(task)) {
        return;
    }
    force = &((struct task_force *)task->force)->speed;
    force->y = 0.0F;
    if (WAIT(work) == 0) {
        switch (work->mode) {
        case LION_WAITING:
            nearest = player_nearest(&work->pos);
            if (njDistanceP2P(&player_work[nearest]->pos, &work->pos) <
                    lion_start_distance &&
                casino_any_above(work->pos.y - START_BELOW)) {
                work->mode = LION_LEFT_RISING;
                work->timer = 0;
            } else {
                /* @hack spelling: the offsets set through a cursor over their
                 * floats, which SHC steps with ADD #4,R4 (0x0c910db2 to
                 * 0x0c910dd2). through the fields, 42 of lion_run's 730 words
                 * differ */
                at = PARTS(work)->right_offset.elements;
                *at++ = 0.0F;
                *at++ = 0.0F;
                *at = 0.0F;
                at = PARTS(work)->left_offset.elements;
                *at++ = 0.0F;
                *at++ = 0.0F;
                *at = 0.0F;
                at = PARTS(work)->middle_offset.elements;
                *at++ = 0.0F;
                *at++ = lion_middle_height;
                *at = 0.0F;
            }
            break;
        case LION_LEFT_RISING:
            if (TASK_TIMER(work) > lion_side_rise_steps) {
                work->mode = LION_LEFT_SLAMMING;
                work->timer = 0;
                WAIT(work) = lion_side_hold_steps;
            } else {
                phase = (Float)TASK_TIMER(work) / lion_side_rise_steps;
                height = QUARTER_SINE(phase) * lion_side_lift;
                if (player->ground_object == &PARTS(work)->left) {
                    force->y = height - PARTS(work)->left_offset.point.y;
                }
                PARTS(work)->left_offset.point.y = height;
            }
            break;
        case LION_LEFT_SLAMMING:
            if (TASK_TIMER(work) > lion_side_slam_steps) {
                work->mode = LION_RIGHT_RISING;
                work->timer = 0;
                WAIT(work) = lion_rest_steps;
                lion_slam_dust(0, work);
                lion_slam(0, work);
                PARTS(work)->left_bounce_speed += lion_bounce_kick;
            } else {
                phase = 1.0F - ((Float)TASK_TIMER(work) / lion_side_slam_steps);
                height = QUARTER_SINE(phase) * lion_side_lift;
                PARTS(work)->left_offset.point.y = height;
            }
            break;
        case LION_RIGHT_RISING:
            if (TASK_TIMER(work) > lion_side_rise_steps) {
                work->mode = LION_RIGHT_SLAMMING;
                work->timer = 0;
                WAIT(work) = lion_side_hold_steps;
            } else {
                phase = (Float)TASK_TIMER(work) / lion_side_rise_steps;
                height = QUARTER_SINE(phase) * lion_side_lift;
                if (player->ground_object == &PARTS(work)->right) {
                    force->y = height - PARTS(work)->right_offset.point.y;
                }
                PARTS(work)->right_offset.point.y = height;
            }
            break;
        case LION_RIGHT_SLAMMING:
            if (TASK_TIMER(work) > lion_side_slam_steps) {
                work->mode = LION_MIDDLE_SINKING;
                work->timer = 0;
                WAIT(work) = lion_rest_steps;
                lion_slam_dust(1, work);
                lion_slam(1, work);
                PARTS(work)->right_bounce_speed += lion_bounce_kick;
            } else {
                phase = 1.0F - ((Float)TASK_TIMER(work) / lion_side_slam_steps);
                height = QUARTER_SINE(phase) * lion_side_lift;
                PARTS(work)->right_offset.point.y = height;
            }
            break;
        case LION_MIDDLE_SINKING:
            if (TASK_TIMER(work) > lion_middle_sink_steps) {
                /* @hack spelling: the cue through a local, which has
                 * sound_play's address load ahead of it (0x0c91103c,
                 * 0x0c911040). as SE_MIDDLE_DOWN, the cue loads first, and 14
                 * of lion_run's 730 words differ */
                int se = SE_MIDDLE_DOWN;

                sound_play(se, 0, 0, 0);
                work->mode = LION_MIDDLE_RISING;
                work->timer = 0;
                WAIT(work) = lion_middle_down_steps;
            } else {
                phase = (Float)TASK_TIMER(work) / lion_middle_sink_steps;
                height = (1.0F - QUARTER_SINE(phase)) * lion_middle_height;
                PARTS(work)->middle_offset.point.y = height;
            }
            break;
        case LION_MIDDLE_RISING:
            if (TASK_TIMER(work) > lion_middle_rise_steps) {
                work->mode = LION_WAITING;
                work->timer = 0;
                WAIT(work) = lion_rest_steps;
            } else {
                phase =
                    1.0F - ((Float)TASK_TIMER(work) / lion_middle_rise_steps);
                height = (1.0F - QUARTER_SINE(phase)) * lion_middle_height;
                if (player->ground_object == &PARTS(work)->middle) {
                    force->y = height - PARTS(work)->middle_offset.point.y;
                }
                PARTS(work)->middle_offset.point.y = height;
            }
            break;
        /* @hack redundant: default: break;, which leaves a NOP in the delay
         * slot of LION_MIDDLE_RISING's branch out (BRA at 0x0c911124). without
         * it, SHC moves the case's last store into the slot, and 3 of
         * lion_run's 730 words differ */
        default:
            break;
        }
    } else {
        WAIT(work)--;
        if (!WAIT(work)) {
            work->timer = 0;
        }
    }
    if (work->mode == LION_RIGHT_SLAMMING || work->mode == LION_LEFT_SLAMMING) {
        if (work->mode == LION_RIGHT_SLAMMING) {
            work->collision->shapes[SHAPE_RIGHT_BOX].flags &= ~SHAPE_OFF;
            work->collision->shapes[SHAPE_RIGHT_HURT].flags &= ~SHAPE_OFF;
            work->collision->shapes[SHAPE_LEFT_HURT].flags |= SHAPE_OFF;
            work->collision->shapes[SHAPE_LEFT_BOX].flags |= SHAPE_OFF;
        } else {
            work->collision->shapes[SHAPE_RIGHT_BOX].flags |= SHAPE_OFF;
            work->collision->shapes[SHAPE_RIGHT_HURT].flags |= SHAPE_OFF;
            work->collision->shapes[SHAPE_LEFT_HURT].flags &= ~SHAPE_OFF;
            work->collision->shapes[SHAPE_LEFT_BOX].flags &= ~SHAPE_OFF;
        }
    } else {
        work->collision->shapes[SHAPE_RIGHT_BOX].flags |= SHAPE_OFF;
        work->collision->shapes[SHAPE_RIGHT_HURT].flags |= SHAPE_OFF;
        work->collision->shapes[SHAPE_LEFT_HURT].flags |= SHAPE_OFF;
        work->collision->shapes[SHAPE_LEFT_BOX].flags |= SHAPE_OFF;
    }
    if (PARTS(work)->middle_offset.point.y > lion_middle_height * 0.5F) {
        work->collision->shapes[SHAPE_MIDDLE_HURT].flags &= ~SHAPE_OFF;
    } else {
        work->collision->shapes[SHAPE_MIDDLE_HURT].flags |= SHAPE_OFF;
    }
    /* @hack redundant: a store to the right box's flags that never runs. SHC
     * counts its references, which keep player_get's result in R8
     * (0x0c910ce4). they also give the right and left boxes' byte offsets in
     * the shapes, 156 and 208, R5 and R6 (0x0c911176, 0x0c91116e). without
     * it, the result goes to the stack, the offsets trade, and 157 of 730
     * words differ */
    if (0) {
        work->collision->shapes[SHAPE_RIGHT_BOX].flags |= SHAPE_OFF;
    }
    work->collision->shapes[SHAPE_RIGHT_BOX].center.y =
        lion_shapes[SHAPE_RIGHT_BOX].center.y +
        PARTS(work)->right_offset.point.y;
    work->collision->shapes[SHAPE_LEFT_BOX].center.y =
        lion_shapes[SHAPE_LEFT_BOX].center.y + PARTS(work)->left_offset.point.y;
    work->collision->shapes[SHAPE_RIGHT_HURT].center.y =
        lion_shapes[SHAPE_RIGHT_HURT].center.y +
        PARTS(work)->right_offset.point.y;
    work->collision->shapes[SHAPE_LEFT_HURT].center.y =
        lion_shapes[SHAPE_LEFT_HURT].center.y +
        PARTS(work)->left_offset.point.y;
    collision_register(work);
    PARTS(work)->left.pos[1] =
        PARTS(work)->left_offset.point.y + PARTS(work)->left_bounce;
    PARTS(work)->right.pos[1] =
        PARTS(work)->right_offset.point.y + PARTS(work)->right_bounce;
    PARTS(work)->middle.pos[1] = PARTS(work)->middle_offset.point.y;
    casino_world_test_near(task, REACH);
    if (!casino_any_above(work->pos.y - TEST_BELOW)) {
        work->flags &= ~WORLD_TESTED;
    }
    PARTS(work)->left_bounce += PARTS(work)->left_bounce_speed;
    PARTS(work)->left_bounce_speed =
        (PARTS(work)->left_bounce_speed * lion_bounce_keep) -
        (PARTS(work)->left_bounce * lion_bounce_spring);
    if (PARTS(work)->left_bounce < 0.0F) {
        PARTS(work)->left_bounce = 0.0F;
        PARTS(work)->left_bounce_speed = -PARTS(work)->left_bounce_speed;
    }
    PARTS(work)->right_bounce += PARTS(work)->right_bounce_speed;
    PARTS(work)->right_bounce_speed =
        (PARTS(work)->right_bounce_speed * lion_bounce_keep) -
        (PARTS(work)->right_bounce * lion_bounce_spring);
    if (PARTS(work)->right_bounce < 0.0F) {
        PARTS(work)->right_bounce = 0.0F;
        PARTS(work)->right_bounce_speed = -PARTS(work)->right_bounce_speed;
    }
    /* @hack spelling: the test for the wrap to 0, written <= 0 for the image's
     * CMP/PL (0x0c9113f2). TASK_TIMER is a Uint16, so <= 0 holds only at 0.
     * the decrement undoes the wrap, which holds the timer at 65535. as == 0,
     * the test is TST, and 2 of lion_run's 730 words differ */
    if (++TASK_TIMER(work) <= 0) {
        --TASK_TIMER(work);
    }
    if (++SHIFT_STEP(work) >= lion_shift_steps * 2) {
        SHIFT_STEP(work) = 0;
    }
}

static void lion_draw(struct task *task)
{
    struct task_work *work = task->work;
    int phase;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, REACH)) {
        return;
    }
    phase = SHIFT_STEP(work) / lion_shift_steps;
    if (phase > 1) {
        phase = 0;
    }
    if (phase) {
        uv_shift_use(lion_shifts[phase - 1], 2);
    }
    casino_set_texture(CASINO_TEXTURES_ACTS_12);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    njPushMatrix(NULL);
    njTranslateV(NULL, (NJS_VECTOR *)PARTS(work)->middle.pos);
    draw_model(PARTS(work)->middle.model);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslateV(NULL, (NJS_VECTOR *)PARTS(work)->right.pos);
    draw_model(PARTS(work)->right_model);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslateV(NULL, (NJS_VECTOR *)PARTS(work)->node_2.pos);
    draw_model(PARTS(work)->node_2.model);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslateV(NULL, (NJS_VECTOR *)PARTS(work)->left.pos);
    draw_model(PARTS(work)->left_model);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslateV(NULL, (NJS_VECTOR *)PARTS(work)->node_4.pos);
    draw_model(PARTS(work)->node_4.model);
    njPopMatrix(1);
    njPopMatrix(1);
    if (phase) {
        uv_shift_restore(lion_shifts[0], 2);
    }
}
