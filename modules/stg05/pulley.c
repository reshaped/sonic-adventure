/* a pulley that carries player 0 from a path's start to its end. its cable
 * is strung between the path's points */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/helpers.h"
#include "path/path.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "stg05/pulley.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_TEXLIST obj_mountain_texlist;
/* the pulley's root model and its nodes. they draw one at a time, so that
 * the wheel can turn */
extern NJS_MODEL pulley_model;
extern NJS_OBJECT pulley_node_1_object;
extern NJS_OBJECT pulley_node_2_object;
extern NJS_OBJECT pulley_node_3_object;
extern NJS_OBJECT pulley_wheel_object;
extern NJS_OBJECT pulley_node_5_object;
extern NJS_MODEL pulley_cable_model;

/* the vars path_create_tasks sets, as path_vars lays them out. distance,
 * which starts at 0, takes the place of wait */
struct pulley_vars {
    Float distance; /* the pulley's distance along the path */
    Sint16 stage_act; /* the stage and act the task was made for */
    struct path *path;
};

#define VARS(work) ((struct pulley_vars *)&(work)->vars)
/* the ride's steps so far, which turn the wheel */
#define RIDE_STEPS(work) TASK_TIMER(work)
/* the point that starts the segment holding the path's end, set as the
 * ride ends */
#define END_POINT(work) ((work)->tick)
/* the cable's sag, and the pulley's drop below its segment's line. the ride
 * sets both each step, and no code reads them */
#define SAG(work) ((work)->scl.x)
#define DROP(work) ((work)->scl.y)
/* the path's length, which init copies from the path. the ride ends past
 * it */
#define PATH_LENGTH(work) ((work)->scl.z)

enum pulley_mode { PULLEY_INIT, PULLEY_WAIT, PULLEY_RIDE, PULLEY_DONE };

/* outside the ride, the pulley draws only while player 0 is nearer than
 * this */
#define PULLEY_NEAR 100.0F
#define PULLEY_SPEED 8.0F /* along the path, each step of the ride */
#define PULLEY_HANG 15.5F /* how far below the pulley player 0 hangs */
/* player 0's speed as the ride ends: PULLEY_LEAVE_SPEED along its y angle,
 * and PULLEY_LEAVE_RISE up */
#define PULLEY_LEAVE_SPEED 2.2F
#define PULLEY_LEAVE_RISE 1.0F
/* the cable's sag at the middle of a segment, as a share of the segment's
 * span */
#define PULLEY_SAG 0.1F
#define PULLEY_WHEEL_TURN 20 /* each step of the ride, in degrees */
/* the cable's scale across x and y. the model is 2 long along z, so a z
 * scale of half the cable's length spans it */
#define PULLEY_CABLE_WIDTH 0.36F

/* the wheel's turn after the ride's steps so far */
#define WHEEL_TURN(work)                                                       \
    (-(Angle)(RIDE_STEPS(work) * PULLEY_WHEEL_TURN * ANGLE_PER_DEGREE))

/* the ride's sound. each step of the ride refreshes it */
#define SE_PULLEY 132

/* where player 0 hangs from the pulley. no code reads it */
/* NOLINTNEXTLINE(clang-diagnostic-unused-const-variable) */
static const NJS_POINT3 pulley_hang = {0.0F, -PULLEY_HANG, 0.0F};

/* while the pulley waits, any touch of this box hangs player 0 from the
 * pulley */
static struct collision_shape pulley_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_FIRM(PUSH_PLAYERS),
    {0.0F, -5.0F, 0.0F},
    5.0F,
    10.0F,
    5.0F,
    {0},
    0,
    NULL,
};

static void pulley_place(struct path *path, Float distance,
                         struct task_work *work);
static void pulley_hold(NJS_POINT3 *pos, struct task_work *work);
static void pulley_draw(struct task *task);
static void pulley_ride(struct task *task);
static void pulley_draw_span(struct task *task, Uint32 index, Uint8 parked);
static void pulley_draw_cable(NJS_POINT3 *from, NJS_POINT3 *to);

void pulley_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player;
    Uint32 point;

    if (stage_get_stage_act() != VARS(work)->stage_act) {
        task_kill(task);
        return;
    }
    switch (work->mode) {
    case PULLEY_INIT:
        collision_init(task, &pulley_shape, COLLISION_OBJECT, 1, 1);
        pulley_place(VARS(work)->path, VARS(work)->distance, work);
        VARS(work)->distance = 0.0F;
        RIDE_STEPS(work) = 0;
        work->mode = PULLEY_WAIT;
        PATH_LENGTH(work) = VARS(work)->path->length;
        break;
    case PULLEY_WAIT:
        if (work->collision->flags & COLLISION_HIT) {
            player_set_mode(0, PLAYER_MODE_HANG_ON);
            work->mode = PULLEY_RIDE;
        }
        collision_register(work);
        player = player_work[0];
        if (njDistanceP2P(&work->pos, &player->pos) < PULLEY_NEAR) {
            Uint32 index;

            VARS(work)->distance = 0.0F;
            pulley_place(VARS(work)->path, VARS(work)->distance, work);
            pulley_draw(task);
            path_find_segment(VARS(work)->path, VARS(work)->distance, &index);
            pulley_draw_span(task, index, 0);
            pulley_draw_span(task, index + 1, 0);
        }
        break;
    case PULLEY_RIDE: {
        Uint32 index;

        path_find_segment(VARS(work)->path, VARS(work)->distance, &index);
        pulley_draw_span(task, index + 1, 0);
        pulley_draw_span(task, index + 2, 0);
        pulley_ride(task);
        VARS(work)->distance += PULLEY_SPEED;
        if (VARS(work)->distance > PATH_LENGTH(work)) {
            Angle heading;
            Uint32 last;

            work->mode = PULLEY_DONE;
            player = player_work[0];
            heading = -player->ang.y;
            player_set_mode(0, PLAYER_MODE_RESET);
            player_set_speed(0, njCos(heading) * PULLEY_LEAVE_SPEED,
                             PULLEY_LEAVE_RISE,
                             -(njSin(heading) * PULLEY_LEAVE_SPEED));
            path_find_segment(VARS(work)->path, PATH_LENGTH(work), &last);
            END_POINT(work) = last;
        }
        sound_play_for(SE_PULLEY, (int)work, 1, 0, 10);
    } break;
    case PULLEY_DONE:
        player = player_work[0];
        if (njDistanceP2P(&work->pos, &player->pos) < PULLEY_NEAR) {
            point = END_POINT(work);
            pulley_draw_span(task, point - 2, 1);
        }
        break;
    }
}

/* place work at distance along the path, facing along it */
static void pulley_place(struct path *path, Float distance,
                         struct task_work *work)
{
    struct path_sample sample;
    NJS_VECTOR ahead;
    Angle ang;

    sample.distance = distance;
    if (path_sample(path, &sample)) {
        work->pos.x = sample.pos.x;
        work->pos.y = sample.pos.y;
        work->pos.z = sample.pos.z;
        work->ang.x = sample.smooth_x;
        work->ang.z = sample.smooth_z;
        njPushMatrix(&_unit_matrix_);
        TURN_VIA(njRotateX, -work->ang.x, ang);
        TURN_VIA(njRotateZ, -work->ang.z, ang);
        njCalcVector(NULL, &sample.ahead, &ahead);
        work->ang.y = (Angle)(atan2f(ahead.x, ahead.z) * ANGLE_PER_RADIAN);
        njPopMatrix(1);
    }
}

/* hang player 0 PULLEY_HANG below pos, at the pulley's y angle */
static void pulley_hold(NJS_POINT3 *pos, struct task_work *work)
{
    struct task_work *player = player_work[0];

    player->pos.x = pos->x;
    player->pos.y = pos->y - PULLEY_HANG;
    player->pos.z = pos->z;
    player_set_angles(0, 0, work->ang.y, 0);
}

static void pulley_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *wheel;
    Angle ang;

    njSetTexture(&obj_mountain_texlist);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y + 0x4000, ang);
    draw_model(&pulley_model);
    njPushMatrix(NULL);
    draw_tree(&pulley_node_1_object);
    draw_tree(&pulley_node_2_object);
    draw_tree(&pulley_node_3_object);
    draw_tree(&pulley_node_5_object);
    wheel = &pulley_wheel_object;
    njPushMatrix(NULL);
    njTranslate(NULL, wheel->pos[0], wheel->pos[1], wheel->pos[2]);
    TURN_VIA(njRotateZ, wheel->ang[2], ang);
    TURN_VIA(njRotateX, wheel->ang[0], ang);
    TURN_VIA(njRotateY, wheel->ang[1], ang);
    TURN_VIA(njRotateX, WHEEL_TURN(work), ang);
    draw_model(wheel->model);
    njPopMatrix(1);
    njPopMatrix(1);
    njPopMatrix(1);
}

/* move the pulley along the path to its distance less PULLEY_SPEED, or to 0.
 * it drops below the line between its segment's two points by the cable's
 * sag there. the sag is a half sine wave across the segment, SAG(work) deep
 * at the middle. player 0 hangs below the new place. the pulley and its
 * cables to the two points draw at the old one */
static void pulley_ride(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 from;
    NJS_POINT3 to;
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    NJS_POINT3 copy;
    NJS_POINT3 pos;
    /* @hack redundant: origin set and never read; without it, the function
     * is 18 bytes shorter and 46 of 480 words differ */
    NJS_POINT3 origin = {0.0F, 0.0F, 0.0F};
    NJS_POINT3 cable;
    struct path_sample sample;
    struct angles from_ang;
    struct angles to_ang;
    struct angles pos_ang;
    NJS_VECTOR ahead;
    Float from_length;
    Float along;
    Float to_length;
    Float from_distance;
    Float to_distance;
    Float span;
    Float sag;
    Float drop;
    Uint32 index;
    Uint32 first;
    Uint32 next;
    NJS_OBJECT *wheel;
    Angle ang;
    /* @hack spelling: njDistanceP2P through a pointer, which keeps its
     * address in a register of its own and njRoundOff's in R10 (0x0c9084b8);
     * called by name, R13 takes njRoundOff's and 30 of 480 words differ */
    Float (*distance)(NJS_POINT3 *p1, NJS_POINT3 *p2);

    path_find_segment(VARS(work)->path, VARS(work)->distance, &index);
    first = njRoundOff(index);
    /* @hack redundant: a select that gives first either way, which keeps the
     * image's test of first - 1 (CMP/PZ R5 at 0x0c9084ec). the code without
     * it falls short of the source's row, which the link refuses; without it,
     * count unmeasured */
    next = (first = (int)(first - 1) < 0 ? 0 : first) + 1;
    path_get_distance(VARS(work)->path, first, &from_distance);
    path_get_distance(VARS(work)->path, next, &to_distance);
    sample.distance = from_distance;
    if (path_sample(VARS(work)->path, &sample)) {
        from.x = sample.pos.x;
        from.y = sample.pos.y;
        from.z = sample.pos.z;
        from_ang.x = sample.smooth_x;
        from_ang.z = sample.smooth_z;
    }
    sample.distance = to_distance;
    if (path_sample(VARS(work)->path, &sample)) {
        to.x = sample.pos.x;
        to.y = sample.pos.y;
        to.z = sample.pos.z;
        to_ang.x = sample.smooth_x;
        to_ang.z = sample.smooth_z;
    }
    sample.distance = VARS(work)->distance - PULLEY_SPEED;
    if (sample.distance < 0.0F) {
        sample.distance = 0.0F;
    }
    if (path_sample(VARS(work)->path, &sample)) {
        pos.x = sample.pos.x;
        pos.y = sample.pos.y;
        pos.z = sample.pos.z;
        work->ang.x = pos_ang.x = sample.smooth_x;
        work->ang.z = pos_ang.z = sample.smooth_z;
        njPushMatrix(&_unit_matrix_);
        TURN_VIA(njRotateX, -work->ang.x, ang);
        TURN_VIA(njRotateZ, -work->ang.z, ang);
        njCalcVector(NULL, &sample.ahead, &ahead);
        work->ang.y = (Angle)(atan2f(ahead.x, ahead.z) * ANGLE_PER_RADIAN);
        njPopMatrix(1);
    }
    along = (distance = njDistanceP2P)(&from, &pos);
    /* this call's result, the distance from to to pos, goes unread */
    distance(&to, &pos);
    span = distance(&from, &to);
    sag = span * PULLEY_SAG;
    SAG(work) = sag;
    drop = sag * njSin((Angle)(along * 180.0F / span * ANGLE_PER_DEGREE));
    DROP(work) = drop;
    pos.y -= drop;
    /* each cable's length is halved and rounded below, and both results go
     * unread */
    from_length = distance(&from, &pos);
    njRoundOff(from_length * 0.5F);
    to_length = distance(&to, &pos);
    njRoundOff(to_length * 0.5F);
    /* @hack redundant: each end of the cable copied to copy, which is never
     * read; without them, the function is 36 bytes shorter and 206 of 480
     * words differ */
    copy = from;
    cable = from;
    pulley_draw_cable(&cable, &work->pos);
    copy = work->pos;
    cable = work->pos;
    pulley_draw_cable(&cable, &to);
    njSetTexture(&obj_mountain_texlist);
    if (work->mode == PULLEY_RIDE) {
        pulley_hold(&pos, work);
    }
    RIDE_STEPS(work)++;
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN_VIA(njRotateY, work->ang.y + 0x4000, ang);
    draw_model(&pulley_model);
    njPushMatrix(NULL);
    draw_tree(&pulley_node_1_object);
    draw_tree(&pulley_node_2_object);
    draw_tree(&pulley_node_3_object);
    draw_tree(&pulley_node_5_object);
    wheel = &pulley_wheel_object;
    njPushMatrix(NULL);
    njTranslate(NULL, wheel->pos[0], wheel->pos[1], wheel->pos[2]);
    TURN_VIA(njRotateZ, wheel->ang[2], ang);
    TURN_VIA(njRotateX, wheel->ang[0], ang);
    TURN_VIA(njRotateY, wheel->ang[1], ang);
    TURN_VIA(njRotateX, WHEEL_TURN(work), ang);
    draw_model(wheel->model);
    njPopMatrix(1);
    njPopMatrix(1);
    njPopMatrix(1);
    work->pos = pos;
}

/* NOLINTNEXTLINE(clang-diagnostic-unused-const-variable) */
static const NJS_POINT3 dat_0cadc938 = {0.0F, 10.0F, 0.0F};
/* NOLINTNEXTLINE(clang-diagnostic-unused-const-variable) */
static const NJS_POINT3 dat_0cadc944 = {0.0F, 0.0F, 0.0F};
static const NJS_POINT3 pulley_origin = {0.0F, 0.0F, 0.0F};

/* draw the cable from the path's point index to the next. with parked 1,
 * draw the pulley at rest on the next too */
static void pulley_draw_span(struct task *task, Uint32 index, Uint8 parked)
{
    struct task_work *work = task->work;
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    NJS_POINT3 copy;
    NJS_POINT3 from;
    NJS_POINT3 to;
    struct path_sample sample;
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    NJS_POINT3 origin;
    Float from_distance;
    Float to_distance;
    Uint32 first;
    Uint32 next;
    NJS_OBJECT *wheel;
    Angle ang;

    first = njRoundOff(index);
    /* @hack redundant: the same select, whose test of first - 1 is CMP/PZ R5
     * at 0x0c9089ca; without it, count unmeasured */
    first = (int)(first - 1) < 0 ? 0 : first;
    next = first + 1;
    path_get_distance(VARS(work)->path, first, &from_distance);
    path_get_distance(VARS(work)->path, next, &to_distance);
    /* @hack redundant: pulley_origin copied to origin, which is never read;
     * without it, the function is 18 bytes shorter and 192 of 254 words
     * differ */
    origin = pulley_origin;
    sample.distance = from_distance;
    /* NOLINTNEXTLINE(clang-diagnostic-sometimes-uninitialized) */
    if (path_sample(VARS(work)->path, &sample)) {
        from.x = sample.pos.x;
        from.y = sample.pos.y;
        from.z = sample.pos.z;
    }
    sample.distance = to_distance;
    if (path_sample(VARS(work)->path, &sample)) {
        to.x = sample.pos.x;
        to.y = sample.pos.y;
        to.z = sample.pos.z;
    }
    if (parked == 1) {
        njSetTexture(&obj_mountain_texlist);
        njPushMatrix(NULL);
        njTranslate(NULL, to.x, to.y, to.z);
        TURN_VIA(njRotateY, work->ang.y + 0x4000, ang);
        draw_model(&pulley_model);
        njPushMatrix(NULL);
        draw_tree(&pulley_node_1_object);
        draw_tree(&pulley_node_2_object);
        draw_tree(&pulley_node_3_object);
        draw_tree(&pulley_node_5_object);
        wheel = &pulley_wheel_object;
        njPushMatrix(NULL);
        njTranslate(NULL, wheel->pos[0], wheel->pos[1], wheel->pos[2]);
        TURN_VIA(njRotateZ, wheel->ang[2], ang);
        TURN_VIA(njRotateX, wheel->ang[0], ang);
        TURN_VIA(njRotateY, wheel->ang[1], ang);
        TURN_VIA(njRotateX, WHEEL_TURN(work), ang);
        draw_model(wheel->model);
        njPopMatrix(1);
        njPopMatrix(1);
        njPopMatrix(1);
    }
    /* @hack redundant: the span's start copied to copy, which is never
     * read; without it, the function is 20 bytes shorter and 27 of 254
     * words differ */
    copy = from;
    pulley_draw_cable(&from, &to);
}

/* draw the cable model at the middle of from and to. it aims along them,
 * stretched to their distance */
static void pulley_draw_cable(NJS_POINT3 *from, NJS_POINT3 *to)
{
    NJS_POINT3 middle;
    NJS_VECTOR dir;
    /* @hack redundant: origin set and never read, which gives the frame 52
     * bytes (ADD #-52,R15 at 0x0c908c48); without it, the frame is 40, the
     * function is 18 bytes shorter and 118 of 123 words differ */
    NJS_VECTOR origin = {0.0F, 0.0F, 0.0F};
    Float length;
    /* @hack type: volatile on half, which keeps it in the frame and the
     * frame at 52 bytes (ADD #-52,R15 at 0x0c908c48); without it, the frame
     * is 48, the function is 8 bytes shorter and 68 of 123 words differ */
    volatile Float half;
    Angle ang_x;
    Angle ang_y;
    /* @hack spelling: the model through a local, which keeps
     * pulley_cable_model's address in R4 for the store of r (0x0c908d04); by
     * name, the pool holds r's address, the function is 2 bytes shorter and
     * 24 of 123 words differ */
    NJS_MODEL *model;

    middle.x = (from->x + to->x) * 0.5F;
    middle.y = (from->y + to->y) * 0.5F;
    middle.z = (from->z + to->z) * 0.5F;
    dir.x = from->x;
    dir.y = from->y;
    dir.z = from->z;
    length = njDistanceP2P(from, to);
    njSubVector(&dir, to);
    vector_aim_z(&dir, &ang_x, &ang_y);
    half = length * 0.5F;
    njSetTexture(&obj_mountain_texlist);
    njPushMatrix(NULL);
    njTranslate(NULL, middle.x, middle.y, middle.z);
    TURN(njRotateX, ang_x);
    TURN(njRotateY, ang_y);
    model = &pulley_cable_model;
    /* draw_model's view test reads the model's radius, which takes half the
     * cable's length */
    model->r = half;
    njScale(NULL, PULLEY_CABLE_WIDTH, PULLEY_CABLE_WIDTH, half);
    draw_model(&pulley_cable_model);
    njPopMatrix(1);
}
