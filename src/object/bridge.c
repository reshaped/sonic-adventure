/* the rope bridge that objects of STAGE_WINDY, STAGE_MOUNTAIN and
 * STAGE_ICECAP build, such as O TURI1. planks hang between posts, and ropes
 * run from plank to plank. a copy of the stage's model, in the collision
 * world, moves its points with the planks */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/world.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "effect/dust.h"
#include "object/bridge.h"
#include "object/placement.h"
#include "player/player.h"
#include "system/task.h"
#include "system/util.h"

/* the bridge destroys itself once the camera and player 0 are both more
 * than 2000 from it. placement_try_leave_by takes the range squared */
#define BRIDGE_LEAVE_RANGE (2000.0F * 2000.0F)

#define BRIDGE_WORLD_FLAGS (WORLD_LARGE | WORLD_TURNED | WORLD_MOVES)

#define VARS(work) BRIDGE_VARS(work)
#define PLANK(work) PLANK_VARS(work)
#define COUNT(work) BRIDGE_PLANK_COUNT(work)

/* a falling plank's work->submode */
enum plank_mode { PLANK_WAIT, PLANK_FALL };

/* a falling plank's timer counts its steps of falling from 0. a step that
 * finds the count above PLANK_FALL_LIMIT kills the plank. that is the 92nd
 * step, which finds 91 */
#define PLANK_FALL_LIMIT 90

/* the force bridge_swing adds to both planks of each neighboring pair. a
 * plank between two others takes it twice a step, and the first and last
 * once. it is negative, as a plank's force counts upward */
#define PLANK_WEIGHT (-0.3F)
/* the player's weight: 68 times PLANK_WEIGHT. SHC folds the product to the
 * image's 0xc1a33332, where -20.4F would give 0xc1a33333 */
#define PLAYER_WEIGHT (PLANK_WEIGHT * 68.0F)
/* standing off the bridge's middle twists the planks under the player.
 * each unit off adds this share of the player's weight to the twist */
#define PLAYER_TWIST 0.06F

/* the plank the collapse spreads out from. a plank d planks from it gets
 * a tick of COLLAPSE_WAIT plus d * d * COLLAPSE_SPREAD, rounded down. so
 * the planks from 34 to 46 all get COLLAPSE_WAIT and fall together, and
 * the rest follow, fewer to a step farther out */
#define COLLAPSE_MIDDLE 40
#define COLLAPSE_WAIT 2
#define COLLAPSE_SPREAD 0.025F

/* a post's height at the bridge's ends, and how much shorter the posts at
 * its middle stand. between them the shortfall follows a sine */
#define POST_HEIGHT 33.0F
#define POST_SAG 24.0F

static void bridge_draw(struct task *task);
static void bridge_step(struct task *task);
static void bridge_setup(struct task *task);
static void bridge_draw_whole(struct task_work *work);
static void bridge_draw_broken(struct task_work *work);
static void bridge_weigh_player(struct task_work *work);
static void bridge_weigh(struct task_work *work, Float x, Float z);
static void bridge_swing(struct task_work *work);
static void bridge_move_points(struct task_work *work);
static void bridge_destroy(struct task *task);

static const NJS_VECTOR plank_dust_speed = {-2.0F, 0.0F, -0.5F};
static const NJS_POINT3 plank_origin = {0.0F, 0.0F, 0.0F};
static const NJS_VECTOR collapse_dust_speed = {-2.0F, 0.0F, -0.5F};

/* the params bridge_init gives parts without any */
static struct bridge_params bridge_default_params = {
    {170.0F, 2.5F, -0.12F, 16.0F, 11.5F, 0.04F, 0.0065F}, 17.5F, {-1.3F, 0.0F}};

void bridge_edit_draw(struct task_work *work)
{
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    draw_model(VARS(work)->parts->model);
    njPopMatrix(1);
}

void bridge_init(struct task *task)
{
    bridge_setup(task);
    task->step = bridge_step;
    task->destructor = bridge_destroy;
    task->draw = bridge_draw;
}

void bridge_step_whole(struct task *task)
{
    struct task_work *work = task->work;

    bridge_draw_whole(work);
    bridge_weigh_player(work);
    bridge_swing(work);
    bridge_move_points(work);
    if (player_in_range(&work->pos,
                        (VARS(work)->object->model->r * 2.0F) + 20.0F)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
    placement_try_leave_by(task, BRIDGE_LEAVE_RANGE);
    /* @hack redundant: a return where the function ends, which makes the stack
     * frame with ADD #-4,R15 and stores task in it (0x8c0a133c, 0x8c0a1340).
     * without it, SHC pushes task with MOV.L R4,@-R15, the function runs 2
     * bytes short and 48 of 49 words differ */
    /* NOLINTNEXTLINE(readability-redundant-control-flow) */
    return;
}

void bridge_step_broken(struct task *task)
{
    struct task_work *work = task->work;

    bridge_draw_broken(work);
    bridge_weigh_player(work);
    bridge_swing(work);
    bridge_move_points(work);
    if (player_in_range(&work->pos, VARS(work)->object->model->r + 800.0F)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
    placement_try_leave_by(task, BRIDGE_LEAVE_RANGE);
    /* @hack redundant: a return where the function ends, which makes the stack
     * frame with ADD #-4,R15 and stores task in it (0x8c0a13fc, 0x8c0a1400).
     * without it, SHC pushes task with MOV.L R4,@-R15 and 44 of 45 words
     * differ */
    /* NOLINTNEXTLINE(readability-redundant-control-flow) */
    return;
}

void bridge_plank_step(struct task *task)
{
    NJS_VECTOR speed;
    NJS_POINT3 pos;
    struct task_work *work = task->work;
    struct plank_motion *motion = (struct plank_motion *)task->motion;
    Float scale = work->scl.x;

    njSetTexture(PLANK(work)->texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    TURN(njRotateX, work->ang.x);
    njScale(NULL, scale, scale, scale);
    draw_model(PLANK(work)->model);
    njPopMatrix(1);
    switch (work->submode) {
    case PLANK_WAIT:
        if (work->tick-- == 0) {
            work->submode++;
            speed = plank_dust_speed;
            pos = work->pos;
            pos.y += 5.0F;
            pos.x -= 20.0F;
            dust_create(&pos, &speed, 5.0F);
        }
        break;
    case PLANK_FALL:
        motion->speed.y -= 0.1F;
        VECTOR_ADD(&work->pos, &motion->speed);
        work->ang.x += motion->spin_x;
        work->ang.y += motion->spin_y;
        if (TASK_TIMER(work)++ > PLANK_FALL_LIMIT) {
            task_kill(task);
        }
        break;
    }
}

void bridge_collapse(struct task_work *work)
{
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    Angle ang;
    NJS_POINT3 *points;
    Float scale;
    NJS_POINT3 origin;
    NJS_POINT3 pos;
    NJS_VECTOR speed;
    struct bridge_plank *planks;
    NJS_POINT3 *foot;
    struct task *child;
    struct task_work *cwork;
    struct plank_motion *motion;
    int i;
    int corner;
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    Float post;

    planks = VARS(work)->planks;
    points = VARS(work)->object->model->points;
    scale = VARS(work)->parts->scale;
    /* @hack redundant: ang, set and never read, which keeps work in R13
     * (MOV R4,R13 at 0x8c0a163a). without it, work and rand's address trade
     * R13 and R12, and 17 of 281 words differ */
    ang = work->ang.y;
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    for (i = 1; i < COUNT(work) - 1; i++) {
        /* @hack redundant: post, set and never read, which keeps 40 in R9
         * and 44 in R10 and leaves post's address unused in R3 (0x8c0a1686,
         * 0x8c0a1690, 0x8c0a16a2). without it, 40 and 44 trade, the
         * function runs 4 bytes short and 223 of 281 words differ */
        post = planks[i].post;
        corner = planks[i].corner[CORNER_FOOT_PLUS_Z];
        foot = &points[corner];
        njPushMatrix(NULL);
        njTranslate(NULL, foot->x, planks[i].height - 1.5F, 0.0F);
        origin = plank_origin;
        njCalcPoint(NULL, &origin, &pos);
        child = task_create(TASK_WORK | TASK_MOTION, 5, bridge_plank_step);
        if (child != NULL) {
            cwork = child->work;
            motion = (struct plank_motion *)child->motion;
            cwork->pos = pos;
            cwork->scl.x = scale;
            cwork->ang.x = planks[i].tilt * ANGLE_PER_DEGREE;
            cwork->ang.y = work->ang.y;
            PLANK(cwork)->model = VARS(work)->parts->plank;
            PLANK(cwork)->texlist = VARS(work)->parts->texlist;
            /* @hack redundant: ang, set again and never read, which loads
             * the y spin's scale into FR0 and copies FR12's -2.5F into FR2
             * (0x8c0a17c0, 0x8c0a17c2). without it, the two trade and 2 of
             * 281 words differ */
            ang = cwork->ang.y;
            /* @hack spelling: the whole wait in each arm, which SHC joins at
             * the LDS. the joined tail keeps the distance in R2 and
             * COLLAPSE_SPREAD in FR14 (0x8c0a1740, 0x8c0a1756). written once
             * after the if, the distance takes R5, COLLAPSE_SPREAD loads at
             * its use and 57 of 281 words differ */
            if (i < COLLAPSE_MIDDLE) {
                cwork->tick =
                    (int)((Float)(COLLAPSE_MIDDLE - i) *
                          (Float)(COLLAPSE_MIDDLE - i) * COLLAPSE_SPREAD) +
                    COLLAPSE_WAIT;
            } else {
                cwork->tick =
                    (int)((Float)(i - COLLAPSE_MIDDLE) *
                          (Float)(i - COLLAPSE_MIDDLE) * COLLAPSE_SPREAD) +
                    COLLAPSE_WAIT;
            }
            motion->speed.x = -3.8F - (rand() * (1.4F * RAND_SCALE));
            motion->speed.y = RANDOM(-1.5F, 0.5F);
            motion->speed.z = -1.0F - (rand() * (0.5F * RAND_SCALE));
            motion->spin_x = RANDOM(-7.5F, 7.5F) * ANGLE_PER_DEGREE;
            motion->spin_y = RANDOM(-2.5F, 2.5F) * ANGLE_PER_DEGREE;
        }
        if (i % 2) {
            speed = collapse_dust_speed;
            pos.y += 5.0F;
            pos.x -= 20.0F;
            dust_create(&pos, &speed, 4.5F);
        }
        njPopMatrix(1);
    }
    njPopMatrix(1);
}

static void bridge_draw(struct task *task)
{
    struct task_work *work = task->work;

    bridge_draw_whole(work);
}

static void bridge_step(struct task *task)
{
    struct task_work *work = task->work;

    if (edit_mode) {
        bridge_edit_draw(work);
    } else {
        bridge_draw_whole(work);
        bridge_weigh_player(work);
        bridge_swing(work);
        bridge_move_points(work);
        if (player_in_range(&work->pos,
                            VARS(work)->object->model->r + 800.0F)) {
            work->flags |= WORLD_TESTED;
        } else {
            work->flags &= ~WORLD_TESTED;
        }
        placement_try_leave_by(task, BRIDGE_LEAVE_RANGE);
    }
}

static void bridge_find_corners(struct task_work *work)
{
    struct bridge_plank *planks = VARS(work)->planks;
    struct bridge_parts *parts = VARS(work)->parts;
    int i;
    int j;

    for (j = 0; j < 4; j++) {
        planks[0].corner[j] = parts->first[0][j];
        planks[1].corner[j] = parts->first[1][j];
        for (i = 2; i < COUNT(work) - 2; i++) {
            planks[i].corner[j] = (i * 4) + parts->middle[j];
        }
        /* @hack spelling: the last two planks through a pointer sum, which SHC
         * multiplies by the count with MULU.W and then takes back 88 bytes
         * with ADD #-88 (0x8c0a1a02, 0x8c0a1a10). as an index, SHC takes 2
         * off the count first and multiplies with MUL.L, the code runs
         * longer and 64 of 103 words differ */
        (planks + COUNT(work) - 2)->corner[j] = parts->last[0][j];
        (planks + COUNT(work) - 1)->corner[j] = parts->last[1][j];
    }
}

static void bridge_reset_planks(struct task_work *work)
{
    struct bridge_plank *planks = VARS(work)->planks;
    struct bridge_parts *parts = VARS(work)->parts;
    struct bridge_plank *plank;
    int i;

    for (i = 0; i < COUNT(work); i++) {
        plank = &planks[i];
        plank->fixed = 0;
        plank->height =
            parts->model->points[plank->corner[CORNER_FOOT_MINUS_Z]].y;
        plank->speed = 0.0F;
        plank->force = 0.0F;
        plank->torque = 0.0F;
        plank->tilt = 0.0F;
        plank->spin = 0.0F;
        plank->post =
            POST_HEIGHT -
            (njSin(i * 180.0F / (COUNT(work) - 1) * ANGLE_PER_DEGREE) *
             POST_SAG);
    }
    planks[0].fixed = planks[COUNT(work) - 1].fixed = 1;
}

static void bridge_setup(struct task *task)
{
    struct task_work *work = task->work;
    struct bridge_parts *parts = VARS(work)->parts;
    struct task_force *force = task->force;

    COUNT(work) = parts->model->nbPoint / 4;
    if (parts->params == NULL) {
        parts->params = &bridge_default_params;
    }
    work->ang.z = 0;
    /* the bridge never moves or turns as a whole, so its force is 0 */
    force->speed.x = 0.0F;
    force->speed.y = 0.0F;
    force->speed.z = 0.0F;
    force->turn.x = 0;
    force->turn.y = 0;
    force->turn.z = 0;
    VARS(work)->planks = heap_alloc(COUNT(work) * sizeof(struct bridge_plank));
    VARS(work)->object = world_object_alloc();
    VARS(work)->object->pos[0] = work->pos.x;
    VARS(work)->object->pos[1] = work->pos.y;
    VARS(work)->object->pos[2] = work->pos.z;
    VARS(work)->object->scl[0] = 1.0F;
    VARS(work)->object->scl[1] = 1.0F;
    VARS(work)->object->scl[2] = 1.0F;
    VARS(work)->object->ang[0] = 0;
    VARS(work)->object->ang[1] = work->ang.y;
    VARS(work)->object->ang[2] = 0;
    VARS(work)->object->model = heap_alloc(sizeof(NJS_MODEL));
    njMemCopy(VARS(work)->object->model, parts->model, sizeof(NJS_MODEL));
    VARS(work)->object->model->points =
        heap_alloc(VARS(work)->object->model->nbPoint * sizeof(NJS_POINT3));
    njMemCopy(VARS(work)->object->model->points, parts->model->points,
              VARS(work)->object->model->nbPoint * sizeof(NJS_POINT3));
    world_add(BRIDGE_WORLD_FLAGS, task, VARS(work)->object);
    bridge_find_corners(work);
    bridge_reset_planks(work);
}

/* draw the two ropes from plank index to the next. one runs between their
 * CORNER_TOP_PLUS_Z points, and one between their CORNER_TOP_MINUS_Z
 * points */
static void bridge_draw_ropes(struct task_work *work, int index)
{
    NJS_POINT3 *points = VARS(work)->object->model->points;
    struct bridge_plank *planks = VARS(work)->planks;
    struct bridge_plank *plank;
    NJS_VECTOR span;
    Float length;
    Angle ang;
    int j;
    int k;

    njSetTexture(VARS(work)->parts->texlist);
    plank = &planks[index];
    /* @hack spelling: index, reused for the first rope's corner, which puts the
     * corner in R5 and the next plank's address in R6 (0x8c0a1c8c,
     * 0x8c0a1c88). with a local of its own, the two trade and 8 of 206
     * words differ */
    index = plank->corner[CORNER_TOP_PLUS_Z];
    k = plank[1].corner[CORNER_TOP_PLUS_Z];
    span.x = points[k].x - points[index].x;
    span.y = points[k].y - points[index].y;
    span.z = points[k].z - points[index].z;
    length = njScalor(&span);
    ang = (Angle)(atan2f(span.y, span.x) * ANGLE_PER_RADIAN);
    njPushMatrix(NULL);
    njTranslate(NULL, points[index].x, points[index].y, points[index].z);
    TURN(njRotateZ, ang);
    njScale(NULL, length, 1.0F, 1.0F);
    draw_model(VARS(work)->parts->rope);
    njPopMatrix(1);
    j = plank->corner[CORNER_TOP_MINUS_Z];
    k = plank[1].corner[CORNER_TOP_MINUS_Z];
    span.x = points[k].x - points[j].x;
    span.y = points[k].y - points[j].y;
    span.z = points[k].z - points[j].z;
    length = njScalor(&span);
    ang = (Angle)(atan2f(span.y, span.x) * ANGLE_PER_RADIAN);
    njPushMatrix(NULL);
    njTranslate(NULL, points[j].x, points[j].y, points[j].z);
    TURN(njRotateZ, ang);
    njScale(NULL, length, 1.0F, 1.0F);
    draw_model(VARS(work)->parts->rope);
    njPopMatrix(1);
}

/* draw the planks, their posts and the ropes */
static void bridge_draw_whole(struct task_work *work)
{
    NJS_POINT3 *points;
    struct bridge_plank *planks;
    NJS_POINT3 *foot;
    Float scale;
    Float post;
    int i;
    int corner;

    planks = VARS(work)->planks;
    points = VARS(work)->object->model->points;
    scale = VARS(work)->parts->scale;
    njSetTexture(VARS(work)->parts->texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    for (i = 1; i < COUNT(work) - 1; i++) {
        post = planks[i].post;
        corner = planks[i].corner[CORNER_FOOT_PLUS_Z];
        foot = &points[corner];
        njPushMatrix(NULL);
        njTranslate(NULL, foot->x, planks[i].height - 1.5F, 0.0F);
        TURN(njRotateX, (Angle)(planks[i].tilt * ANGLE_PER_DEGREE));
        njScale(NULL, scale, scale, scale);
        draw_model(VARS(work)->parts->plank);
        njPopMatrix(1);
        njPushMatrix(NULL);
        njTranslate(NULL, foot->x, foot->y, foot->z);
        njScale(NULL, 1.0F, post, -1.0F);
        draw_model(VARS(work)->parts->post);
        njPopMatrix(1);
        corner = planks[i].corner[CORNER_FOOT_MINUS_Z];
        foot = &points[corner];
        njPushMatrix(NULL);
        njTranslate(NULL, foot->x, foot->y, foot->z);
        njScale(NULL, 1.0F, post, -1.0F);
        draw_model(VARS(work)->parts->post);
        njPopMatrix(1);
    }
    for (i = 0; i < COUNT(work) - 1; i++) {
        bridge_draw_ropes(work, i);
    }
    njPopMatrix(1);
}

/* draw the posts and the ropes, once the planks have fallen */
static void bridge_draw_broken(struct task_work *work)
{
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    Angle ang;
    NJS_POINT3 *points;
    struct bridge_plank *planks;
    NJS_POINT3 *foot;
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    Float scale;
    Float post;
    int corner;
    int i;

    planks = VARS(work)->planks;
    points = VARS(work)->object->model->points;
    /* @hack redundant: scale, set and never read, which loads it into R2
     * and keeps njPushMatrix's address in R10 (0x8c0a2054, 0x8c0a205a).
     * without it, R12 takes the address and 140 of 159 words differ */
    scale = VARS(work)->parts->scale;
    /* @hack redundant: ang, set and never read, which shares TURN's load
     * and stores it at @(4,R15) (0x8c0a206a, 0x8c0a2070). it keeps
     * njTranslate's address in R8 (0x8c0a2084). without it, draw_model's
     * takes R8 and 116 of 159 words differ */
    ang = work->ang.y;
    njSetTexture(VARS(work)->parts->texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    for (i = 1; i < COUNT(work) - 1; i++) {
        post = planks[i].post;
        corner = planks[i].corner[CORNER_FOOT_PLUS_Z];
        foot = &points[corner];
        njPushMatrix(NULL);
        njTranslate(NULL, foot->x, foot->y, foot->z);
        njScale(NULL, 1.0F, post, -1.0F);
        draw_model(VARS(work)->parts->post);
        njPopMatrix(1);
        corner = planks[i].corner[CORNER_FOOT_MINUS_Z];
        foot = &points[corner];
        njPushMatrix(NULL);
        njTranslate(NULL, foot->x, foot->y, foot->z);
        njScale(NULL, 1.0F, post, -1.0F);
        draw_model(VARS(work)->parts->post);
        njPopMatrix(1);
    }
    for (i = 0; i < COUNT(work) - 1; i++) {
        bridge_draw_ropes(work, i);
    }
    njPopMatrix(1);
}

/* set *out to player 0's position in the bridge's frame */
static void bridge_player_pos(struct task_work *work, NJS_VECTOR *out)
{
    NJS_MATRIX matrix;
    NJS_VECTOR offset;
    NJS_VECTOR local;
    struct task_work *player = player_work[0];

    offset.x = player->pos.x - work->pos.x;
    offset.y = player->pos.y - work->pos.y;
    offset.z = player->pos.z - work->pos.z;
    njUnitMatrix(&matrix);
    TURN_MATRIX(njRotateY, &matrix, -work->ang.y);
    njCalcVector(&matrix, &offset, &local);
    out->x = local.x;
    out->y = local.y;
    out->z = local.z;
}

/* 1 while player 0 stands on the bridge, else 0. it stands there while it
 * has PLAYER_ON_LAND set and at lies inside the parts' box. at is its
 * position in the bridge's frame */
/* @hack type: the result as a Uint8, which bridge_weigh_player widens with
 * EXTU.B R0,R0 (0x8c0a2292). as a Bool or an int, it doesn't, and 35 of
 * bridge_weigh_player's 44 words differ */
static Uint8 bridge_player_on(struct task_work *work, NJS_VECTOR *at)
{
    /* @hack spelling: the box through a pointer to the whole array, which SHC
     * holds in R6 (ADD #40,R6 at 0x8c0a21e2). through the parts, it forms
     * the box's address in R1, and 5 of 44 words differ */
    struct bridge_range(*box)[3] = &VARS(work)->parts->box;
    struct task_work *player = player_work[0];

    if ((*box)[0].min < at->x && at->x < (*box)[0].max &&
        (*box)[1].min < at->y && at->y < (*box)[1].max &&
        (*box)[2].min < at->z && at->z < (*box)[2].max &&
        (player->flags & PLAYER_ON_LAND)) {
        return 1;
    }
    return 0;
}

/* while player 0 stands on the bridge, add its weight to the planks under
 * it. work->scl.x and scl.z keep where it stands. work->tick counts its
 * steps there, from 0 to 11 and over again */
static void bridge_weigh_player(struct task_work *work)
{
    NJS_VECTOR at;

    bridge_player_pos(work, &at);
    if (bridge_player_on(work, &at)) {
        work->scl.z = at.z;
        work->scl.x = at.x;
        bridge_weigh(work, at.x, at.z);
        if (work->tick++ > 10) {
            work->tick = 0;
            /* @hack redundant: a return where the arm ends, which gives the arm
             * an epilogue of its own (0x8c0a22c4 to 0x8c0a22ca). without
             * it, the arm branches to the shared one, the function runs 6
             * bytes short and 14 of 44 words differ */
            return;
        }
    } else {
        work->tick = 0;
    }
}

/* add the player's weight at x, z in the bridge's frame to the two planks
 * around x. each takes a share by how near x is to it. standing off the
 * middle, at z, also twists them. an x at or past the last plank adds
 * nothing */
static void bridge_weigh(struct task_work *work, Float x, Float z)
{
    struct bridge_plank *planks = VARS(work)->planks;
    struct bridge_plank *plank;
    struct bridge_plank *next;
    Float place;
    Float twist;
    Float along;
    Float weight;
    Float length;
    Uint16 index;

    length = VARS(work)->parts->length;
    twist = z * PLAYER_TWIST;
    place = x / length;
    place *= COUNT(work) - 1;
    index = place;
    along = njFraction(place);
    weight = PLAYER_WEIGHT;
    if (index < COUNT(work) - 1) {
        plank = &planks[index];
        next = plank + 1;
        plank->force += (1.0F - along) * weight;
        next->force += along * weight;
        plank->torque -= (1.0F - along) * twist * weight;
        next->torque -= along * twist * weight;
    }
}

/* pull each two neighboring planks toward one height and one tilt, and add
 * PLANK_WEIGHT to both. the pairs at the ends pull twice as hard for
 * height. then move each plank that isn't fixed by its force and torque,
 * and clear both */
static void bridge_swing(struct task_work *work)
{
    struct bridge_plank *planks = VARS(work)->planks;
    Float diff;
    Float pull;
    int i;

    for (i = 0; i < COUNT(work) - 1; i++) {
        diff = planks[i + 1].height - planks[i].height;
        /* @hack spelling: the two tests joined by |, which runs both to a
         * bit with MOVT and ORs the bits (0x8c0a2394 to 0x8c0a23a2). with ||,
         * 10 of 163 words differ */
        if ((i == 0) | (i == COUNT(work) - 2)) {
            pull = diff * 32.0F;
        } else {
            pull = diff * 16.0F;
        }
        planks[i].force += pull + PLANK_WEIGHT;
        planks[i + 1].force += PLANK_WEIGHT - pull;
        pull = planks[i + 1].tilt - planks[i].tilt;
        pull *= 11.5F;
        planks[i].torque += pull;
        planks[i + 1].torque += -pull;
    }
    for (i = 0; i < COUNT(work); i++) {
        if (planks[i].fixed == 0) {
            planks[i].speed +=
                (planks[i].force * (1.0F / 22.5F)) - (planks[i].speed * 0.04F);
            if (planks[i].speed < -1.3F) {
                planks[i].speed = -1.3F;
            }
            planks[i].height += planks[i].speed;
            planks[i].spin +=
                (planks[i].torque * 0.04F) - (planks[i].spin * 0.0065F);
            planks[i].tilt += planks[i].spin;
        }
        planks[i].force = 0.0F;
        planks[i].torque = 0.0F;
    }
}

/* move the model's points to the planks. each plank's feet sit width to
 * either side of its middle, turned by its tilt. its posts' tops stand post
 * above its feet */
static void bridge_move_points(struct task_work *work)
{
    struct bridge_params *params = VARS(work)->parts->params;
    Float width = params->width;
    NJS_POINT3 *points = VARS(work)->object->model->points;
    struct bridge_plank *plank = VARS(work)->planks;
    int i;

    for (i = 0; i < COUNT(work); i++, plank++) {
        points[plank->corner[CORNER_FOOT_MINUS_Z]].y =
            plank->height + (width * njSin(plank->tilt * ANGLE_PER_DEGREE));
        points[plank->corner[CORNER_FOOT_MINUS_Z]].z =
            -(width * njCos(plank->tilt * ANGLE_PER_DEGREE));
        points[plank->corner[CORNER_FOOT_PLUS_Z]].y =
            plank->height - (width * njSin(plank->tilt * ANGLE_PER_DEGREE));
        points[plank->corner[CORNER_FOOT_PLUS_Z]].z =
            width * njCos(plank->tilt * ANGLE_PER_DEGREE);
        points[plank->corner[CORNER_TOP_PLUS_Z]].y =
            points[plank->corner[CORNER_FOOT_PLUS_Z]].y + plank->post;
        points[plank->corner[CORNER_TOP_PLUS_Z]].z =
            points[plank->corner[CORNER_FOOT_PLUS_Z]].z;
        points[plank->corner[CORNER_TOP_MINUS_Z]].y =
            points[plank->corner[CORNER_FOOT_MINUS_Z]].y + plank->post;
        points[plank->corner[CORNER_TOP_MINUS_Z]].z =
            points[plank->corner[CORNER_FOOT_MINUS_Z]].z;
    }
}

static void bridge_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->object != NULL) {
        if (VARS(work)->object->model != NULL) {
            if (VARS(work)->object->model->points != NULL) {
                heap_free(VARS(work)->object->model->points);
            }
            heap_free(VARS(work)->object->model);
        }
        world_remove(task, VARS(work)->object);
        world_object_free(VARS(work)->object);
        VARS(work)->object = NULL;
    }
    if (VARS(work)->planks != NULL) {
        heap_free(VARS(work)->planks);
        VARS(work)->planks = NULL;
    }
}
