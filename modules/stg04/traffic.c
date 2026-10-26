/* act 2's traffic: the path task that drives nine cars around traffic_route */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "collision/world.h"
#include "draw/draw.h"
#include "path/path.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "stg04/paths.h"
#include "stg04/police.h"
#include "stg04/traffic.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texlists.h" /* NOLINT(misc-include-cleaner) */
#include "system/util.h"

#define TEXT_1_LENGTH 6
#define TEXT_2_LENGTH 7
#define TEXT_3_LENGTH 6
#define TEXT_4_LENGTH 6
#define TEXT_5_LENGTH 6
#define TEXT_6_LENGTH 9
#define TEXT_7_LENGTH 9
#define TEXT_8_LENGTH 9
#define TEXT_9_LENGTH 7
#define TEXT_10_LENGTH 9
#define TEXT_11_LENGTH 10
#define TEXT_12_LENGTH 9
#define TEXT_13_LENGTH 11
#define TEXT_14_LENGTH 8
#define TEXT_15_LENGTH 4
#define TEXT_16_LENGTH 9
#include "stg04/traffic/text.inc"

/* by kind: the model its cars put in the collision world, and the action that
 * draws them */
extern NJS_MODEL highway_car_0_model;
extern NJS_MODEL highway_car_1_model;
extern NJS_MODEL highway_car_2_model;
extern NJS_MODEL highway_car_3_model;
extern NJS_ACTION highway_car_0_action;
extern NJS_ACTION highway_car_1_action;
extern NJS_ACTION highway_car_2_action;
extern NJS_ACTION highway_car_3_action;
/* the matrix of the car being drawn */
extern NJS_MATRIX traffic_car_matrix;

#define CARS 9
/* traffic_route's length */
#define ROUTE_LENGTH 8112.7251F

#define SE_CARS_NEAR 97 /* while a driving car is near player 0 */
#define SE_CAR_BRAKE 98 /* as a car brakes for player 0 */
#define SE_CAR_HIT 99 /* as a car throws a player */
#define SE_CAR_STOPPED 119 /* once a braking car has stood stopped a while */

/* how near player 0 a driving car keeps SE_CARS_NEAR playing */
#define CAR_NEAR 80.0F

/* the stretch of traffic_route, from SHARP_FROM to SHARP_TO, where cars may
 * turn faster */
#define SHARP_FROM 4200.0F
#define SHARP_TO 5000.0F
/* the most a car turns a step, in degrees for each unit of its speed.
 * SHARP_TURN_RATE is the most on that stretch */
#define TURN_RATE 0.47F
#define SHARP_TURN_RATE 0.67F
/* a car's roll takes ROLL_SHARE of each turn, held within ROLL_MOST degrees
 * either way. it then eases back toward 0 by ROLL_EASE a step */
#define ROLL_SHARE 0.8F
#define ROLL_MOST 6.0F
#define ROLL_EASE 0.3F

/* a car slows for a slower car ahead within FOLLOW_GAP, plus gap_per_speed for
 * each unit of its speed. it then makes for that car's speed, less
 * FOLLOW_SLACK over the gap between them */
#define FOLLOW_GAP 55.0F
#define FOLLOW_SLACK 8.0F
/* a car stops for player 0 within STOP_REACH of traffic_route. player 0 must
 * also be less than STOP_AHEAD ahead of the car along the route */
#define STOP_REACH 18.0F
#define STOP_AHEAD 50.0F
/* the speed above which a car that stops for player 0 brakes */
#define BRAKE_FROM 1.5F
/* a driving car eases toward the speed it makes for. each step it gains
 * SPEED_SHARE of any shortfall, at most GAIN_MOST. it sheds SLOWING_FACTOR
 * times that share of any excess, at most LOSS_MOST */
#define SPEED_SHARE 0.01F
#define GAIN_MOST 0.08F
#define SLOWING_FACTOR 6.0F
#define LOSS_MOST 0.9F
/* a car's brake lights glow for BRAKE_LIGHT_DRAWS draws after traffic_car_drive
 * slows it by more than BRAKE_LIGHT_LEAST */
#define BRAKE_LIGHT_DRAWS 15
#define BRAKE_LIGHT_LEAST 0.001F

/* the speed a braking car loses each step */
#define BRAKE_SLOWING 0.02F
/* a braking car's pitch rises by PITCH_STEP a step while it moves, to at most
 * PITCH_MOST. stopped, the pitch eases back to 0 by PITCH_EASE a step */
#define PITCH_STEP 40
#define PITCH_MOST 0x828
#define PITCH_EASE 200
/* a braking car drives again once it has stood stopped more steps than
 * STOPPED_STEPS. it plays SE_CAR_STOPPED once it has stood stopped
 * STOPPED_SOUND_STEPS steps */
#define STOPPED_STEPS 50.0F
#define STOPPED_SOUND_STEPS 20.0F

/* how far toward the camera from its lamp each lamp's glow is drawn */
#define HEADLIGHT_GLOW_OFFSET 16.0F
#define BRAKE_GLOW_OFFSET 8.0F
/* the frames a car's action advances each step it is drawn, for each unit of
 * its speed */
#define ACTION_PACE 0.4F

/* the collision world tests a car's object only while a player is within its
 * model's radius plus PLAYER_NEAR */
#define PLAYER_NEAR 20.0F
/* a car faster than THROW_FROM throws a player its nose touches. the throw
 * also needs the squared difference of the speeds traffic_car_collide compares
 * to pass THROW_SQUARED. past HURT_SQUARED, the throw hurts the player too */
#define THROW_FROM 0.5F
#define THROW_SQUARED 1.0F
#define HURT_SQUARED 2.0F
/* a thrown player flies THROW_SPEED along the yaw THROW_TURN short of the
 * car's, and THROW_LIFT up. that yaw lies 45 degrees off the car's front,
 * toward its +z. the player's position rises THROW_RISE at once, and its stick
 * is ignored for THROW_STEPS steps */
#define THROW_SPEED 2.7F
#define THROW_TURN 0x2000
#define THROW_LIFT 2.4F
#define THROW_RISE 1.0F
#define THROW_STEPS 60

/* bits of the path task's work->submode */
#define TRAFFIC_NEAR 0x1 /* a car this round was near player 0 */
#define TRAFFIC_HELD 0x2 /* SE_CARS_NEAR is playing */

/* the path task's vars: struct path_vars, with car in wait's place. car is the
 * car traffic_run tests next */
struct traffic_vars {
    struct task *car;
    Sint16 stage_act;
};

#define VARS(work) ((struct traffic_vars *)&(work)->vars)

enum car_mode { CAR_DRIVE, CAR_BRAKE };

/* a car's work: task_work's layout, with fields of the car's own */
struct car_work {
    Sint8 mode;
    Sint8 id; /* its row of traffic_cars */
    Uint8 unk_02;
    Uint8 kind; /* its row of traffic_car_kinds */
    Sint16 flags;
    Uint16 timer; /* how many more draws its brake lights glow */
    Float roll; /* its lean as it turns, in degrees */
    struct car_work *ahead; /* the car ahead of it on traffic_route */
    NJS_OBJECT *world; /* its object in the collision world */
    struct angles ang;
    union point_floats pos;
    Float distance; /* its distance along traffic_route */
    Float speed;
    Float top_speed;
    struct collision_work *collision;
    struct event_work *event;
};

/* fields of a car's motion and extra records that hold the car's own values.
 * PITCH is its pitch as it brakes. STOPPED counts the steps it has stood
 * stopped. STEP_START is where it stood as its step began. FRAME is its
 * action's frame */
#define PITCH(motion) ((motion)->aim.z)
#define STOPPED(motion) ((motion)->gravity)
#define STEP_START(motion) ((motion)->accel)
#define FRAME(task) (*(Float *)(task)->extra)

struct car_kind {
    NJS_MODEL *model; /* what its cars put in the collision world */
    NJS_TEXLIST *texlist;
    NJS_ACTION *action;
    Float unk_0c;
    Float speed; /* its cars' starting speed, and their top speed */
    Float unk_14;
};

struct car_row {
    Sint8 kind;
    Uint8 unk_01[3];
    Float distance; /* where the car starts along traffic_route */
    /* FOLLOW_GAP's growth for each unit of a car's speed. traffic_car_drive
     * reads it from the row the car's kind numbers, not the car's own. only
     * rows 0 to 3's are read */
    Float gap_per_speed;
    Float offset; /* how far the car drives from traffic_route, across it */
};

/* a stretch of traffic_route, and the speed the cars make for in it */
struct speed_zone {
    Float from;
    Float to;
    Float speed;
};

/* a kind's two lamps, as points in the car's own frame. its front faces -x */
struct lamp_pair {
    NJS_POINT3 points[2];
};

static void traffic_run(struct task *task);
static void traffic_destroy(struct task *task);
static void traffic_create_cars(struct task *task);
static void traffic_car_step(struct task *task);
static void traffic_car_follow(struct task *task);
static void traffic_car_drive(struct task *task);
static void traffic_car_brake(struct task *task);
static void traffic_car_draw_headlights(struct car_work *work,
                                        NJS_MATRIX *matrix);
static void traffic_car_draw_brake_lights(struct car_work *work,
                                          NJS_MATRIX *matrix);
static void traffic_car_draw_advance(struct task *task);
static void traffic_car_draw(struct task *task);
static void traffic_car_throw_player(int number, struct car_work *work);
static void traffic_car_collide(struct car_work *work);
static void traffic_car_carry_turn(struct task *task, Angle turn, Sint8 number);
static void traffic_car_carry(struct task *task);
static void fun_0c906022(struct car_work *work);
static void traffic_car_run(struct task *task);
static void traffic_car_destroy(struct task *task);

static const struct lamp_pair traffic_headlights[4] = {
#include "stg04/traffic/traffic_headlights.inc"
};
static const struct lamp_pair traffic_brake_lights[4] = {
#include "stg04/traffic/traffic_brake_lights.inc"
};

static NJS_TEXNAME highway_car_texnames[16] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
};
NJS_TEXLIST highway_car_texlist = {
#include "stg04/traffic/highway_car_texlist.inc"
};

/* by kind: the body, then the nose, which pushes players and enemies.
 * traffic_car_collide can throw a player the nose touches */
/* clang-format off */
static struct collision_shape traffic_car_shapes[4][2] = {
    {{SHAPE_CAPSULE | SHAPE_HURTABLE, SHAPE_GUARD | SHAPE_HURTS_PLAYERS,
      PUSH_FIRM(PUSH_ALL), {-10.0F, -14.0F, 0.0F}, 30.0F, 25.0F, 0.0F,
      {0, 0, 0x4000}, 0, NULL},
     {SHAPE_CYLINDER | SHAPE_HURTABLE, SHAPE_GUARD | SHAPE_HURTS_PLAYERS,
      PUSH_PLAYERS | PUSH_ENEMIES, {-14.0F, 4.0F, 0.0F}, 11.5F, 4.0F, 4.0F,
      {0, 0, 0}, 0, NULL}},
    {{SHAPE_CAPSULE | SHAPE_HURTABLE, SHAPE_GUARD | SHAPE_HURTS_PLAYERS,
      PUSH_FIRM(PUSH_ALL), {-10.0F, -14.0F, 0.0F}, 30.0F, 25.0F, 0.0F,
      {0, 0, 0x4000}, 0, NULL},
     {SHAPE_CYLINDER | SHAPE_HURTABLE, SHAPE_GUARD | SHAPE_HURTS_PLAYERS,
      PUSH_PLAYERS | PUSH_ENEMIES, {-13.0F, 3.0F, 0.0F}, 10.0F, 3.0F, 4.0F,
      {0, 0, 0}, 0, NULL}},
    {{SHAPE_CAPSULE | SHAPE_HURTABLE, SHAPE_GUARD | SHAPE_HURTS_PLAYERS,
      PUSH_FIRM(PUSH_ALL), {-10.0F, -14.0F, 0.0F}, 40.0F, 25.0F, 0.0F,
      {0, 0, 0x4000}, 0, NULL},
     {SHAPE_CYLINDER | SHAPE_HURTABLE, SHAPE_GUARD | SHAPE_HURTS_PLAYERS,
      PUSH_PLAYERS | PUSH_ENEMIES, {-12.0F, 10.5F, 0.0F}, 12.0F, 10.5F, 4.0F,
      {0, 0, 0}, 0, NULL}},
    {{SHAPE_CAPSULE | SHAPE_HURTABLE, SHAPE_GUARD | SHAPE_HURTS_PLAYERS,
      PUSH_FIRM(PUSH_ALL), {-10.0F, -14.0F, 0.0F}, 30.0F, 25.0F, 0.0F,
      {0, 0, 0x4000}, 0, NULL},
     {SHAPE_CYLINDER | SHAPE_HURTABLE, SHAPE_GUARD | SHAPE_HURTS_PLAYERS,
      PUSH_PLAYERS | PUSH_ENEMIES, {-12.0F, 4.0F, 0.0F}, 10.0F, 4.0F, 4.0F,
      {0, 0, 0}, 0, NULL}},
};
/* clang-format on */

static struct car_kind traffic_car_kinds[4] = {
    {&highway_car_0_model, &highway_car_texlist, &highway_car_0_action, 0.025F,
     4.8F, 20.0F},
    {&highway_car_1_model, &highway_car_texlist, &highway_car_1_action, 0.025F,
     4.8F, 20.0F},
    {&highway_car_2_model, &highway_car_texlist, &highway_car_2_action, 0.025F,
     4.8F, 20.0F},
    {&highway_car_3_model, &highway_car_texlist, &highway_car_3_action, 0.025F,
     4.8F, 20.0F},
};

/* the cars' rows, by id. only the first CARS make cars, and no code reads the
 * rest. SHC folds the offsets written as products of 0.3F to the image's
 * values. as literals, 0.9F, -0.9F and -1.5F come out one unit in the last
 * place further from 0 */
static struct car_row traffic_cars[19] = {
    {2, {3, 0, 0}, 500.0F, 95.0F, 2.7F},
    {3, {3, 0, 0}, 1000.0F, 95.0F, 0.3F * 3},
    {0, {4, 0, 0}, 1500.0F, 105.0F, -2.4F},
    {1, {0, 0, 0}, 2000.0F, 95.0F, -0.6F},
    {2, {1, 0, 0}, 2500.0F, 95.0F, 0.0F},
    {1, {3, 0, 0}, 3050.0F, 80.0F, 0.0F},
    {3, {3, 0, 0}, 3350.0F, 95.0F, 0.3F * -3},
    {3, {3, 0, 0}, 3500.0F, 95.0F, 0.3F * 3},
    {0, {0, 0, 0}, 4020.0F, 105.0F, 0.0F},
    {2, {3, 0, 0}, 4300.0F, 95.0F, -0.3F},
    {3, {0, 0, 0}, 4600.0F, 80.0F, 0.0F},
    {1, {1, 0, 0}, 5000.0F, 95.0F, 0.0F},
    {2, {1, 0, 0}, 5500.0F, 95.0F, 0.6F},
    {3, {3, 0, 0}, 6000.0F, 95.0F, 0.3F * 3},
    {0, {1, 0, 0}, 6400.0F, 105.0F, 0.3F * -5},
    {3, {3, 0, 0}, 6950.0F, 80.0F, 0.0F},
    {1, {5, 0, 0}, 7200.0F, 95.0F, -0.3F},
    {2, {1, 0, 0}, 7600.0F, 95.0F, 0.0F},
    {1, {0, 0, 0}, 8000.0F, 95.0F, 0.3F * -3},
};

/* EFF_REGULAR's panlight alone, and the lamps' glow, one 64 by 64 frame of
 * it */
static NJS_TEXLIST traffic_glow_texlist = {
#include "stg04/traffic/traffic_glow_texlist.inc"
};
static NJS_TEXANIM traffic_glow_texanim[1] = {
#include "stg04/traffic/traffic_glow_texanim.inc"
};

static struct speed_zone traffic_speeds[4] = {
    {6200.0F, 7000.0F, 3.2F},
    {1950.0F, 2950.0F, 3.5F},
    {4250.0F, 5000.0F, 2.6F},
    {0.0F, 900.0F, 2.6F},
};

struct route_nearest traffic_act2_nearest;
/* the point of traffic_route nearest player 0 */
static struct route_nearest traffic_route_nearest;
/* player 0's distance from that point, truncated */
static int traffic_route_away;
static NJS_SPRITE traffic_glow;

void traffic_step(struct task *task)
{
    struct task_work *work = task->work;

    traffic_glow.tlist = &traffic_glow_texlist;
    traffic_glow.tanim = traffic_glow_texanim;
    traffic_glow.sx = 0.2F;
    traffic_glow.sy = 0.2F;
    traffic_glow.p.x = 0.0F;
    traffic_glow.p.y = 0.0F;
    traffic_glow.p.z = 0.0F;
    traffic_create_cars(task);
    work->submode = 0;
    TASK_TIMER(work) = 0;
    VARS(work)->car = task->child;
    task->destructor = traffic_destroy;
    task->step = traffic_run;
}

/* the path task's step once traffic_step has made the cars. each step finds
 * player 0's nearest points on highway_act2_route and traffic_route, and tests
 * the next car. a round is CARS steps, one for each car. SE_CARS_NEAR plays
 * while the last round found a driving car near player 0. the cars end once the
 * stage or act isn't the one the task was made for */
static void traffic_run(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    struct route_nearest *nearest = &traffic_act2_nearest;
    struct task *child;
    struct car_work *car;
    Float *from;
    Float *to;
    Float px;
    Float py;
    Float pz;
    Float dx;
    Float dy;
    Float dz;
    Float squared;
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    Float unused;
    int flags;
    int timer;
    int i;

    nearest->distance = path_find_nearest(&highway_act2_route, &player->pos,
                                          &nearest->pos, &nearest->along);
    nearest->stretch = -1;
    for (i = 0; i < POLICE_STRETCHES; i++) {
        if (police_stretches[i][1] <= nearest->along &&
            police_stretches[i][0] >= nearest->along) {
            nearest->stretch = i;
            break;
        }
    }
    nearest = &traffic_route_nearest;
    nearest->distance = path_find_nearest(&traffic_route, &player->pos,
                                          &nearest->pos, &nearest->along);
    traffic_route_away =
        (int)njDistanceP2P(&player_work[0]->pos, &nearest->pos);
    child = VARS(work)->car;
    car = (struct car_work *)child->work;
    /* @hack spelling: the points read through cursors, which FMOV.S @R5+ and
     * @R4+ step from 0x0c904576 and 0x0c904580; without it, SHC loads each
     * float by member at an offset in R0, and 110 of 163 words differ */
    from = VECTOR_FLOATS(&player->pos);
    to = car->pos.elements;
    px = *from;
    from++;
    py = *from;
    from++;
    pz = *from;
    dx = *to;
    to++;
    dy = *to;
    to++;
    dx -= px;
    dz = *to;
    dz -= pz;
    dy -= py;
    /* @hack redundant: a read SHC drops but counts, which keeps the stretches'
     * 7 in R5 at 0x0c904516, the car's task in R6 and its work in R7 at
     * 0x0c904574 and 0x0c904578, and dx, dy and dz in FR6, FR4 and FR5 from
     * 0x0c904580; without it, R5 and R6 trade, as do R6 and R7, the three
     * take FR5, FR6 and FR4, and 26 of 163 words differ */
    unused = ((dy * (dy + dz)) && py && to && to != from && child &&
              child->parent != child)
                 ? work->pos.x
                 : work->pos.y;
    timer = TASK_TIMER(work);
    squared = (dx * dx) + (dy * dy) + (dz * dz);
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    flags = work->submode;
    if (squared < CAR_NEAR * CAR_NEAR && car->mode != CAR_BRAKE) {
        flags |= TRAFFIC_NEAR;
    }
    if (++timer >= CARS) {
        if (flags & TRAFFIC_NEAR) {
            if (!(flags & TRAFFIC_HELD)) {
                sound_play_held(SE_CARS_NEAR, (int)work, 1, 0);
                flags |= TRAFFIC_HELD;
            }
        } else {
            if (flags & TRAFFIC_HELD) {
                sound_stop_id((int)work);
                flags &= ~TRAFFIC_HELD;
            }
        }
        timer = 0;
        flags &= ~TRAFFIC_NEAR;
        VARS(work)->car = task->child;
    } else {
        VARS(work)->car = child->next;
    }
    TASK_TIMER(work) = timer;
    work->submode = flags;
    task_step_children(task);
    if (stage_get_stage_act() != VARS(work)->stage_act) {
        traffic_destroy(task);
    }
}

static void traffic_destroy(struct task *task)
{
    struct task_work *work = task->work;

    task_destroy_children(task);
    if (work->submode & TRAFFIC_HELD) {
        sound_stop_id((int)work);
    }
}

/* make a car for rows CARS - 1 down to 0, each the path task's first child. a
 * car starts at its row's distance, braking from its kind's speed, and stops
 * before it first drives. the car of the next row is ahead of it, and row 0's
 * is ahead of the last */
static void traffic_create_cars(struct task *task)
{
    /* @hack spelling: a copy of task, which with the jump below gives 0x4000
     * R9 and 1 R11 at 0x0c904718 and 0x0c90471a, and keeps 1's load ahead of
     * task's store at 0x0c90471e; without it, that load comes last, at
     * 0x0c904722, and 15 of 489 words differ */
    struct task *parent = task;
    struct task *child;
    struct task *tail;
    struct car_work *work;
    struct path_sample sample;
    int i;
    int kind;

    /* @hack redundant: a jump to the next statement, which with the copy above
     * gives 0x4000 R9 and 1 R11 at 0x0c904718 and 0x0c90471a; without it,
     * the two trade, and 13 of 489 words differ */
    goto create;
create:
    for (i = CARS - 1; i >= 0; i--) {
        child =
            task_create_child(TASK_MOTION | TASK_WORK | TASK_FORCE | TASK_EXTRA,
                              traffic_car_step, parent);
        if (child) {
            work = (struct car_work *)child->work;
            /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
            kind = traffic_cars[i].kind;
            work->id = i;
            work->mode = CAR_BRAKE;
            work->kind = kind;
            work->timer = 0;
            work->distance = traffic_cars[i].distance;
            work->speed = traffic_car_kinds[kind].speed;
            work->top_speed = traffic_car_kinds[kind].speed;
            if (child->next) {
                work->ahead = (struct car_work *)child->next->work;
            }
            sample.distance = traffic_cars[i].distance;
            if (path_sample(&traffic_route, &sample)) {
                work->ang.y = (Angle)(atan2f(sample.ahead.x, sample.ahead.z) *
                                      ANGLE_PER_RADIAN) +
                              NJM_DEG_ANG(90);
                work->pos.point.x = sample.pos.x;
                work->pos.point.y = sample.pos.y;
                work->pos.point.z = sample.pos.z;
            }
        }
    }
    tail = parent->child;
    while (tail->next) {
        tail = tail->next;
    }
    ((struct car_work *)tail->work)->ahead = (struct car_work *)child->work;
}

/* the car's first step. it puts its kind's model in the collision world where
 * the car stands. it gives the car its kind's shapes. traffic_car_run steps it
 * from then on */
static void traffic_car_step(struct task *task)
{
    struct car_work *work = (struct car_work *)task->work;
    NJS_OBJECT *world;

    work->world = world = world_object_alloc();
    world->evalflags = NJD_EVAL_UNIT_SCL | NJD_EVAL_BREAK;
    world->scl[0] = 1.0F;
    world->scl[1] = 1.0F;
    world->scl[2] = 1.0F;
    world->ang[0] = work->ang.x;
    world->ang[1] = work->ang.y;
    world->ang[2] = work->ang.z;
    world->pos[0] = work->pos.point.x;
    world->pos[1] = work->pos.point.y;
    world->pos[2] = work->pos.point.z;
    world->model = traffic_car_kinds[work->kind].model;
    world->child = NULL;
    world->sibling = NULL;
    world_add(WORLD_MOVES, task, world);
    collision_init(task, traffic_car_shapes[work->kind], COLLISION_OBJECT, 1,
                   2);
    task->destructor = traffic_car_destroy;
    task->step = traffic_car_run;
    task->draw = traffic_car_draw;
}

/* place the car at its distance along traffic_route, its row's offset across
 * it. the car tilts as the route does, and turns toward the route's heading.
 * its roll follows its turns */
static void traffic_car_follow(struct task *task)
{
    struct car_work *work = (struct car_work *)task->work;
    struct task_force *force = task->force;
    /* @hack redundant: 320 bytes of frame that no code reads, declared as a
     * Uint8[320], a type that gives only their size. they make the frame the
     * image's 408 bytes (ADD R0,R15 at 0x0c904c2c, R0 -408); without them,
     * the object's code runs 32 bytes short, and 79 of traffic_car_follow's
     * 170 words differ */
    Uint8 unused[320];
    struct path_sample place;
    /* @hack spelling: the sample through a pointer, which the image holds in
     * R13 from 0x0c904c34; without it, SHC reads it from R15, and 135 of 170
     * words differ */
    struct path_sample *sample = &place;
    Angle old_yaw;
    Angle yaw;
    Angle delta;
    Angle turn_step;
    Float degrees;

    sample->distance = work->distance;
    /* path_sample can't fail on traffic_route, which loops. were it to fail,
     * delta would be read unset */
    /* NOLINTNEXTLINE(clang-diagnostic-sometimes-uninitialized) */
    if (path_sample(&traffic_route, sample)) {
        old_yaw = work->ang.y;
        yaw = (Angle)(atan2f(sample->ahead.x, sample->ahead.z) *
                      ANGLE_PER_RADIAN) +
              NJM_DEG_ANG(90);
        yaw = (Uint16)yaw;
        work->ang.x = sample->smooth_x;
        if (work->distance > SHARP_FROM && work->distance < SHARP_TO) {
            turn_step =
                (Angle)(work->speed * (SHARP_TURN_RATE * ANGLE_PER_DEGREE));
        } else {
            turn_step = (Angle)(work->speed * (TURN_RATE * ANGLE_PER_DEGREE));
        }
        work->ang.y = angle_approach(work->ang.y, yaw, turn_step);
        work->ang.z = sample->smooth_z;
        delta = angle_difference(work->ang.y, old_yaw);
        force[0].turn.y = delta;
        work->pos.point.x = sample->pos.x - (traffic_cars[work->id].offset *
                                             njSin(work->ang.y));
        work->pos.point.y = sample->pos.y;
        work->pos.point.z = sample->pos.z - (traffic_cars[work->id].offset *
                                             njCos(work->ang.y));
    }
    yaw = (Uint16)delta;
    degrees = yaw * (360.0F / 65536.0F);
    if (degrees > 180.0F) {
        degrees -= 360.0F;
    }
    work->roll += degrees * ROLL_SHARE;
    if (work->roll > ROLL_MOST) {
        work->roll = ROLL_MOST;
    } else if (work->roll < -ROLL_MOST) {
        work->roll = -ROLL_MOST;
    }
    if (work->roll > ROLL_EASE) {
        work->roll -= ROLL_EASE;
    } else if (work->roll < -ROLL_EASE) {
        work->roll += ROLL_EASE;
    } else {
        work->roll = 0.0F;
    }
}

/* drive the car a step along traffic_route. it makes for its zone's speed, or
 * else its top speed. it makes for less behind a slower car close ahead, as
 * FOLLOW_GAP says. it makes for a stop while player 0 is in its way, as
 * STOP_REACH says. faster than BRAKE_FROM, it brakes for that stop */
static void traffic_car_drive(struct task *task)
{
    struct car_work *work = (struct car_work *)task->work;
    struct car_work *leader = work->ahead;
    Float distance = work->distance;
    Float gap = leader->distance - work->distance;
    Float speed = work->speed;
    Float target = work->top_speed;
    Float acceleration;
    Float player_ahead;
    int i;

    /* @hack redundant: a read SHC drops but counts, which keeps the car ahead's
     * work in R6 at 0x0c904e06 and the zone's row in R7 at 0x0c904e26;
     * without it, the two trade, and 12 of 184 words differ */
    (void)leader->speed;
    if (gap < 0.0F) {
        gap += ROUTE_LENGTH;
    }
    for (i = 0; i < 4; i++) {
        if (distance > traffic_speeds[i].from &&
            distance < traffic_speeds[i].to) {
            target = traffic_speeds[i].speed;
            break;
        }
    }
    if (gap < (traffic_cars[work->kind].gap_per_speed * speed) + FOLLOW_GAP &&
        target > leader->speed) {
        target = leader->speed - (FOLLOW_SLACK / gap);
    }
    if (traffic_route_away < STOP_REACH) {
        player_ahead = traffic_route_nearest.along;
        /* @hack redundant: distance loaded again from work->distance, whose
         * value it holds already, which leaves distance in FR0 and the speed
         * in FR14 from 0x0c904e08; without it, they take FR1 and FR13, and 110
         * of 184 words differ */
        distance = work->distance;
        player_ahead -= distance;
        if ((player_ahead > 0.0F && player_ahead < STOP_AHEAD) ||
            player_ahead < STOP_AHEAD - ROUTE_LENGTH) {
            target = 0.0F;
            if (speed > BRAKE_FROM) {
                work->mode = CAR_BRAKE;
                sound_play(SE_CAR_BRAKE, 0, 0, 0);
            }
        }
    }
    target -= speed;
    acceleration = target * SPEED_SHARE;
    if (acceleration < 0.0F) {
        acceleration *= SLOWING_FACTOR;
    }
    if (acceleration > GAIN_MOST) {
        acceleration = GAIN_MOST;
    } else if (acceleration < -LOSS_MOST) {
        acceleration = -LOSS_MOST;
    }
    if (acceleration < -BRAKE_LIGHT_LEAST) {
        work->timer = BRAKE_LIGHT_DRAWS;
    } else if (work->timer < 0) {
        /* @bug logic: the timer is unsigned, so CMP/PZ at 0x0c904f3c
         * always holds and BT at 0x0c904f3e skips this store */
        work->timer = 0;
    }
    work->speed += acceleration;
    if (work->speed > work->top_speed) {
        work->speed = work->top_speed;
    } else if (work->speed < 0.0F) {
        work->speed = 0.0F;
    }
    work->distance += work->speed;
    if (work->distance > ROUTE_LENGTH) {
        work->distance -= ROUTE_LENGTH;
    }
}

/* slow the car to a stop, its pitch rising. stopped, its pitch eases back to 0.
 * it drives again once it has stood stopped more than STOPPED_STEPS steps */
static void traffic_car_brake(struct task *task)
{
    struct car_work *work = (struct car_work *)task->work;
    struct task_motion *motion = task->motion;
    Float speed = work->speed;
    /* @hack spelling: stopped read through task, which keeps task in R12 and
     * motion in R11 at 0x0c904fe8 and 0x0c904ff8, and the speed in FR14 and
     * stopped in FR15; without it, each pair trades, and 23 of 87 words
     * differ */
    Float stopped = STOPPED(task->motion);
    Angle pitch = PITCH(motion);
    Float distance = work->distance;
    Float length;

    if (speed != 0.0F) {
        speed -= BRAKE_SLOWING;
        if (pitch > PITCH_MOST - PITCH_STEP) {
            pitch = PITCH_MOST - PITCH_STEP;
        }
        pitch += PITCH_STEP;
        if (speed < 0.0F) {
            speed = 0.0F;
            stopped = 0.0F;
        }
    } else {
        if (pitch > 0) {
            pitch -= PITCH_EASE;
            if (pitch < 0) {
                pitch = 0;
            }
        }
        stopped += 1.0F;
        if (stopped > STOPPED_STEPS) {
            stopped = 0.0F;
            work->mode = CAR_DRIVE;
        } else if (stopped == STOPPED_SOUND_STEPS) {
            sound_play(SE_CAR_STOPPED, 0, 0, 0);
        }
    }
    distance += speed;
    /* @hack spelling: the length through the route's address, which keeps
     * traffic_route itself in the pool word at 0x0c9051e8; without it, SHC
     * folds the 4 into that word, and 22 of 87 words differ */
    length = (&traffic_route)->length;
    if (distance > length) {
        distance -= length;
    }
    work->speed = speed;
    /* @hack spelling: stopped stored through task, which the image loads again
     * at 0x0c9050a8; without it, 82 of 87 words differ */
    STOPPED(task->motion) = stopped;
    PITCH(motion) = pitch;
    work->distance = distance;
}

/* draw a glow HEADLIGHT_GLOW_OFFSET toward the camera from each headlight.
 * facing is the cosine between the car's front and the way to the camera.
 * below 0, the glows' alpha and scale are 0. from 0, the alpha is
 * (2 facing - 1) cubed plus 0.1. it runs from -0.9 to 1.1, and is below 0 for
 * a facing under about 0.27. the scale is 1.1 times the alpha */
static void traffic_car_draw_headlights(struct car_work *work,
                                        NJS_MATRIX *matrix)
{
    /* @hack type: a volatile copy of matrix, which keeps it at R15+4,
     * stored at 0x0c9050e2, and the angles' spill at R15+0, with
     * njColorBlendingMode's address in R10 and njPushMatrix's in R12. as a
     * plain copy, the matrix takes R8 and the frame is 4 bytes smaller;
     * without it, the slots trade, the addresses take R9 and R11, and 36 of
     * 421 words differ */
    NJS_MATRIX *volatile held_matrix = matrix;
    NJS_POINT3 origin;
    NJS_POINT3 sprite_pos;
    NJS_POINT3 left;
    NJS_POINT3 right;
    NJS_VECTOR direction;
    NJS_VECTOR axis;
    NJS_VECTOR view;
    /* @hack type: the first lamp's pointer in a record, which gives it R15+16
     * of its own, stored at 0x0c905296 for the second lamp at 0x0c905372;
     * without it, the pointer takes the angles' R15+0, SHC copies the base
     * twice, and 265 of 421 words differ */
    struct {
        const NJS_POINT3 *first;
    } held;
    /* @hack type: volatile on saved, which keeps the kind at R15+12,
     * stored at 0x0c9050ea, and njColorBlendingMode's address in R10 at
     * 0x0c9050e0; without it, the kind takes R15+8, and 13 of 421 words
     * differ */
    const NJS_POINT3 *volatile saved;
    const NJS_POINT3 *light;
    /* @hack type: volatile on kind, which keeps it at R15+12, stored at
     * 0x0c9050ea and read at 0x0c90527e; without it, the kind takes R15+0 and
     * the matrix R15+8, and 224 of 421 words differ */
    volatile int kind = work->kind;
    Angle angle;
    Float facing;
    Float intensity;

    origin.z = 0.0F;
    origin.y = 0.0F;
    origin.x = 0.0F;
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    njSetTexture(&traffic_glow_texlist);
    direction.x = -1.0F;
    direction.y = 0.0F;
    direction.z = 0.0F;
    njPushMatrix(&_unit_matrix_);
    TURN_VIA(njRotateZ, work->ang.z, angle);
    TURN_VIA(njRotateX, work->ang.x, angle);
    TURN_VIA(njRotateY, work->ang.y, angle);
    TURN_VIA(njRotateX, (Angle)(work->roll * ANGLE_PER_DEGREE), angle);
    njCalcVector(NULL, &direction, &axis);
    njPopMatrix(1);
    view.x = work->pos.point.x - camera_work->pos.x;
    view.y = work->pos.point.y - camera_work->pos.y;
    view.z = work->pos.point.z - camera_work->pos.z;
    njUnitVector(&view);
    facing = -njInnerProduct(&axis, &view);
    if (facing < 0.0F) {
        intensity = 0.0F;
    } else {
        facing = (facing - 0.5F) * 2.0F;
        intensity = (facing * facing * facing) + 0.1F;
    }
    draw_set_material(intensity, 1.0F, 1.0F, 0.95F);
    njFogDisable();
    intensity *= 1.1F;
    njPushMatrix(&_unit_matrix_);
    njMultiMatrix(NULL, held_matrix);
    /* @hack spelling: the comma, which keeps the setup with its call; without
     * it, as a statement of its own, njTranslate's and the lamps' pool words
     * trade at 0x0c9054a4 and 0x0c9054a8, as do the loads of x and y at
     * 0x0c90529c, and 4 of 421 words differ */
    /* @hack redundant: the store to saved, at 0x0c905294 to R15+8, which
     * the function never reads; without it, 250 of 421 words differ */
    light = held.first = saved = traffic_headlights[kind].points,
    njTranslate(NULL, light->x, light->y, light->z);
    njCalcPoint(NULL, &origin, &left);
    njPopMatrix(1);
    direction.x = camera_work->pos.x - left.x;
    direction.y = camera_work->pos.y - left.y;
    direction.z = camera_work->pos.z - left.z;
    njUnitVector(&direction);
    sprite_pos.x = left.x + (direction.x * HEADLIGHT_GLOW_OFFSET);
    sprite_pos.y = left.y + (direction.y * HEADLIGHT_GLOW_OFFSET);
    sprite_pos.z = left.z + (direction.z * HEADLIGHT_GLOW_OFFSET);
    njPushMatrix(NULL);
    njTranslate(NULL, sprite_pos.x, sprite_pos.y, sprite_pos.z);
    TURN_VIA(njRotateY, camera_work->ang.y, angle);
    TURN_VIA(njRotateX, camera_work->ang.x, angle);
    njScale(NULL, intensity, intensity, intensity);
    njDrawSprite3D(&traffic_glow, 0, NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
    njPopMatrix(1);
    njPushMatrix(held_matrix);
    light = held.first + 1;
    /* @hack redundant: an arm never taken, which keeps the second lamp's base
     * in R3; without it, SHC copies the base to R2 first, at 0x0c905378, and
     * 150 of 421 words differ */
    njTranslate(NULL, light->x, (0 ? 2.0F : light->y), light->z);
    njCalcPoint(NULL, &origin, &right);
    njPopMatrix(1);
    direction.x = camera_work->pos.x - right.x;
    direction.y = camera_work->pos.y - right.y;
    direction.z = camera_work->pos.z - right.z;
    njUnitVector(&direction);
    sprite_pos.x = right.x + (direction.x * HEADLIGHT_GLOW_OFFSET);
    sprite_pos.y = right.y + (direction.y * HEADLIGHT_GLOW_OFFSET);
    sprite_pos.z = right.z + (direction.z * HEADLIGHT_GLOW_OFFSET);
    njPushMatrix(NULL);
    njTranslate(NULL, sprite_pos.x, sprite_pos.y, sprite_pos.z);
    TURN_VIA(njRotateY, camera_work->ang.y, angle);
    TURN_VIA(njRotateX, camera_work->ang.x, angle);
    njScale(NULL, intensity, intensity, intensity);
    njDrawSprite3D(&traffic_glow, 0, NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
    njPopMatrix(1);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njFogEnable();
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    /* @hack redundant: a never-run test of saved, which keeps saved at R15+8
     * and the kind at R15+12, which otherwise trade; without it, 3 of 421 words
     * differ */
    if (0) {
        if (saved) {
        }
    }
}

/* while the brake lights' draws last, count one off. draw a red glow
 * BRAKE_GLOW_OFFSET toward the camera from each. the glows fade as the draws
 * run out. their alpha is 0 while the car's front faces the camera */
static void traffic_car_draw_brake_lights(struct car_work *work,
                                          NJS_MATRIX *matrix)
{
    NJS_POINT3 origin;
    NJS_POINT3 sprite_pos;
    NJS_POINT3 left;
    NJS_POINT3 right;
    NJS_VECTOR direction;
    NJS_VECTOR axis;
    NJS_VECTOR view;
    /* @hack type: the lamps' base through a record, which SHC stores at
     * 0x0c9056ac, to R15+12, and never reads; without it, as a plain pointer,
     * the frame is 4 bytes smaller, and 261 of 429 words differ */
    struct {
        const NJS_POINT3 *first;
    } saved;
    const NJS_POINT3 *light;
    const NJS_POINT3 *base;
    int kind = work->kind;
    Angle angle;
    Float facing;
    Float intensity;

    if (work->timer <= 0) {
        return;
    }
    --work->timer;
    origin.z = 0.0F;
    origin.y = 0.0F;
    origin.x = 0.0F;
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    njSetTexture(&traffic_glow_texlist);
    njFogDisable();
    direction.x = -1.0F;
    direction.z = 0.0F;
    direction.y = 0.0F;
    njPushMatrix(&_unit_matrix_);
    TURN_VIA(njRotateZ, work->ang.z, angle);
    TURN_VIA(njRotateX, work->ang.x, angle);
    TURN_VIA(njRotateY, work->ang.y, angle);
    TURN_VIA(njRotateX, (Angle)(work->roll * ANGLE_PER_DEGREE), angle);
    njCalcVector(NULL, &direction, &axis);
    njPopMatrix(1);
    view.x = work->pos.point.x - camera_work->pos.x;
    view.y = work->pos.point.y - camera_work->pos.y;
    view.z = work->pos.point.z - camera_work->pos.z;
    njUnitVector(&view);
    /* 1 with the car's back toward the camera, -1 with its front */
    facing = njInnerProduct(&axis, &view);
    if (facing < 0.0F) {
        intensity = 0.0F;
    } else {
        intensity = (facing + 0.1F) * work->timer * 0.1F;
    }
    draw_set_material(intensity * 0.7F, 1.0F, 0.2F, 0.0F);
    /* @hack redundant: a never-run test of intensity, which keeps the kind at
     * R15+0 and the matrix at R15+8, stored at 0x0c905500 and 0x0c9054fa, which
     * otherwise trade; without it, 9 of 429 words differ */
    if (0) {
        if (intensity) {
        }
    }
    intensity = (intensity * 0.4F) + 0.15F;
    njPushMatrix(matrix);
    /* @hack spelling: the inner comma, which stores the record, then base to
     * R15+4, at 0x0c9056ac and 0x0c9056ae, for the second lamp to read at
     * 0x0c90578a; without it, as light = base = saved.first = ..., SHC stores
     * the record alone, and 229 of 429 words differ */
    /* @hack spelling: the outer comma, which keeps the setup with its call;
     * without it, as a statement of its own, njTranslate's and njUnitVector's
     * addresses trade R9 and R8, and 258 of 429 words differ */
    light =
        (saved.first = traffic_brake_lights[kind].points, base = saved.first),
    njTranslate(NULL, light->x, light->y, light->z);
    njCalcPoint(NULL, &origin, &left);
    njPopMatrix(1);
    direction.x = camera_work->pos.x - left.x;
    direction.y = camera_work->pos.y - left.y;
    direction.z = camera_work->pos.z - left.z;
    njUnitVector(&direction);
    sprite_pos.x = left.x + (direction.x * BRAKE_GLOW_OFFSET);
    sprite_pos.y = left.y + (direction.y * BRAKE_GLOW_OFFSET);
    sprite_pos.z = left.z + (direction.z * BRAKE_GLOW_OFFSET);
    njPushMatrix(NULL);
    njTranslate(NULL, sprite_pos.x, sprite_pos.y, sprite_pos.z);
    TURN_VIA(njRotateY, camera_work->ang.y, angle);
    TURN_VIA(njRotateX, camera_work->ang.x, angle);
    njScale(NULL, intensity, intensity, intensity);
    njDrawSprite3D(&traffic_glow, 0, NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
    njPopMatrix(1);
    njPushMatrix(matrix);
    light = base + 1;
    /* @hack redundant: an arm never taken, which keeps the second lamp's base
     * in R3; without it, SHC copies the base to R2 first, at 0x0c905790, and
     * 133 of 429 words differ */
    njTranslate(NULL, light->x, (0 ? 2.0F : light->y), light->z);
    njCalcPoint(NULL, &origin, &right);
    njPopMatrix(1);
    direction.x = camera_work->pos.x - right.x;
    direction.y = camera_work->pos.y - right.y;
    direction.z = camera_work->pos.z - right.z;
    njUnitVector(&direction);
    sprite_pos.x = right.x + (direction.x * BRAKE_GLOW_OFFSET);
    sprite_pos.y = right.y + (direction.y * BRAKE_GLOW_OFFSET);
    sprite_pos.z = right.z + (direction.z * BRAKE_GLOW_OFFSET);
    njPushMatrix(NULL);
    njTranslate(NULL, sprite_pos.x, sprite_pos.y, sprite_pos.z);
    TURN_VIA(njRotateY, camera_work->ang.y, angle);
    TURN_VIA(njRotateX, camera_work->ang.x, angle);
    njScale(NULL, intensity, intensity, intensity);
    njDrawSprite3D(&traffic_glow, 0, NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
    njPopMatrix(1);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njFogEnable();
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    /* @hack redundant: never-run calls, which SHC counts: they keep
     * njColorBlendingMode's address in R11 and njPushMatrix's in R10 at
     * 0x0c905516 and 0x0c905540, which otherwise trade, and with the fourth pop
     * njUnitVector's in R8 at 0x0c9055be; without it, 26 of 429 words differ */
    if (0) {
        njPopMatrix(1);
        njPopMatrix(1);
        njPopMatrix(1);
        njPopMatrix(1);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR,
                            NJD_COLOR_BLENDING_INVSRCALPHA);
    }
}

/* draw the car and its lamps, then advance its action. it does neither while
 * frame_step isn't 0, or past the far clip from the camera */
static void traffic_car_draw_advance(struct task *task)
{
    struct car_work *work = (struct car_work *)task->work;
    struct task_motion *motion = task->motion;
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    int id = work->id;
    unsigned int index;
    struct car_row *row;
    int number;
    struct car_kind *kind;
    Float far_clip;
    Angle angle;

    index = (Uint16)id;
    /* @hack redundant: updates that cancel, which with the test below keep id
     * in R5 apart from index in R4, at 0x0c9058fa and 0x0c9058fc; without
     * them, both take R4, and 2 of 195 words differ */
    index++;
    index--;
    /* @hack redundant: a never-run test of id, which, with the updates above,
     * keeps id in R5 apart from index in R4 (0x0c9058fa, 0x0c9058fc), where
     * otherwise SHC drops the copy and the code after moves up; without it, 182
     * of 195 words differ */
    if (0) {
        if (id) {
        }
    }
    /* @hack redundant: a jump to the next statement, which keeps the far clip's
     * address in R2, the rows' in R3 and number in R12, from 0x0c9058f8;
     * without it, the addresses take R1 and R2, number goes to the frame, and
     * 115 of 195 words differ */
    goto lookup;
lookup:
    row = &traffic_cars[index];
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    number = row->kind;
    far_clip = _nj_far_clip_;
    if (njDistanceP2P(&work->pos.point, &camera_work->pos) > far_clip) {
        return;
    }
    if (frame_step) {
        return;
    }
    njPushMatrix(NULL);
    njPushMatrix(&_unit_matrix_);
    njTranslate(NULL, work->pos.point.x, work->pos.point.y, work->pos.point.z);
    TURN_VIA(njRotateZ, work->ang.z, angle);
    TURN_VIA(njRotateX, work->ang.x, angle);
    TURN_VIA(njRotateY, work->ang.y, angle);
    TURN_VIA(njRotateX, (Angle)(work->roll * ANGLE_PER_DEGREE), angle);
    if (work->mode == CAR_BRAKE) {
        TURN_VIA(njRotateZ, PITCH(motion), angle);
    }
    njGetMatrix(&traffic_car_matrix);
    njPopMatrix(1);
    traffic_car_draw_headlights(work, &traffic_car_matrix);
    traffic_car_draw_brake_lights(work, &traffic_car_matrix);
    kind = &traffic_car_kinds[number];
    njSetTexture(kind->texlist);
    njMultiMatrix(NULL, &traffic_car_matrix);
    draw_action(kind->action, FRAME(task));
    njPopMatrix(1);
    FRAME(task) += work->speed * ACTION_PACE;
    if (kind->action->motion->nbFrame <= FRAME(task)) {
        FRAME(task) -= traffic_car_kinds[number].action->motion->nbFrame;
    }
}

/* traffic_car_draw_advance without the advance */
static void traffic_car_draw(struct task *task)
{
    struct car_work *work = (struct car_work *)task->work;
    struct task_motion *motion = task->motion;
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    int id = work->id;
    unsigned int index;
    struct car_row *row;
    Sint8 kind;
    Float far_clip;
    Angle angle;

    index = (Uint16)id;
    /* @hack redundant: updates that cancel, as in traffic_car_draw_advance,
     * which keep id in R5 apart from index in R4 at 0x0c905af6 and
     * 0x0c905af8; without them, both take R4, and 2 of 150 words differ */
    index++;
    index--;
    /* @hack redundant: the same as in traffic_car_draw_advance, which keeps id
     * in R5 apart from index in R4 (0x0c905af6, 0x0c905af8), where otherwise
     * SHC drops the copy and the code after moves up; without it, 140 of 150
     * words differ */
    if (0) {
        if (id) {
        }
    }
    /* @hack redundant: a jump to the next statement, as in
     * traffic_car_draw_advance, which keeps the far clip's address in R2 and
     * the rows' in R3, at 0x0c905aee and 0x0c905af4; without it, the two
     * trade, and 13 of 150 words differ */
    goto lookup;
lookup:
    row = &traffic_cars[index];
    kind = row->kind;
    far_clip = _nj_far_clip_;
    if (njDistanceP2P(&work->pos.point, &camera_work->pos) > far_clip) {
        return;
    }
    if (frame_step) {
        return;
    }
    njPushMatrix(NULL);
    njPushMatrix(&_unit_matrix_);
    njTranslate(NULL, work->pos.point.x, work->pos.point.y, work->pos.point.z);
    TURN_VIA(njRotateZ, work->ang.z, angle);
    TURN_VIA(njRotateX, work->ang.x, angle);
    TURN_VIA(njRotateY, work->ang.y, angle);
    TURN_VIA(njRotateX, (Angle)(work->roll * ANGLE_PER_DEGREE), angle);
    if (work->mode == CAR_BRAKE) {
        TURN_VIA(njRotateZ, PITCH(motion), angle);
    }
    njGetMatrix(&traffic_car_matrix);
    njPopMatrix(1);
    traffic_car_draw_headlights(work, &traffic_car_matrix);
    traffic_car_draw_brake_lights(work, &traffic_car_matrix);
    njSetTexture(traffic_car_kinds[kind].texlist);
    njMultiMatrix(NULL, &traffic_car_matrix);
    draw_action(traffic_car_kinds[kind].action, FRAME(task));
    njPopMatrix(1);
}

/* throw player number off the car along the throw's yaw. the player faces that
 * yaw and flies forward, or faces the opposite one and flies backward */
static void traffic_car_throw_player(int number, struct car_work *work)
{
    struct task_work *player = player_work[number];
    NJS_POINT3 velocity;
    Angle rotation[3];

    rotation[0] = 0;
    rotation[1] = work->ang.y - THROW_TURN;
    rotation[2] = 0;
    /* @bug logic: the test sets the throw's yaw against the player's
     * ang.y, R5 at the JSR to angle_distance at 0x0c905cb0.
     * player_send_flying gives the player 0x4000 less the yaw it is passed,
     * so the two are counted different ways. the test doesn't measure the
     * player's turn to the throw's yaw */
    if (angle_distance(rotation[1], player->ang.y) > 0x4000) {
        rotation[1] += 0x8000;
        velocity.x = -THROW_SPEED;
    } else {
        velocity.x = THROW_SPEED;
    }
    velocity.y = THROW_LIFT;
    velocity.z = 0.0F;
    player_send_flying(number, &velocity, rotation, THROW_STEPS);
    player->pos.y += THROW_RISE;
    sound_play(SE_CAR_HIT, 0, 0, 0);
}

/* keep the car's object in the collision world with the car. while the car is
 * faster than THROW_FROM, its nose throws a player it touches, as THROW_SQUARED
 * says */
static void traffic_car_collide(struct car_work *work)
{
    NJS_OBJECT *world = work->world;

    world->pos[0] = work->pos.point.x;
    world->pos[1] = work->pos.point.y;
    world->pos[2] = work->pos.point.z;
    world->ang[0] = work->ang.x;
    world->ang[1] = work->ang.y;
    world->ang[2] = work->ang.z;
    if (work->speed > THROW_FROM && (work->flags & COLLISION_TOUCHING) &&
        work->collision->own_shape == 1 &&
        work->collision->hit->kind == COLLISION_PLAYER) {
        int player = PLAYER_VARS(work->collision->hit->owner->work)->player;
        Angle heading = player_work[player]->ang.y;
        NJS_VECTOR *speed = &player_motion[player]->speed;
        Float cosine = njCos(heading);
        Float sine = njSin(heading);
        /* @bug logic: x and z are the car's speed along its +x, its back,
         * in the world, less the player's world speed turned by its heading
         * alone into the frame it faces (FMUL FR14,FR2 at 0x0c905df8, FMUL
         * FR15,FR1 at 0x0c905e00, FSUB at 0x0c905e02 and 0x0c905e06). the two
         * frames differ, so squared measures neither speed against the
         * other */
        Float x = (work->speed * njCos(work->ang.y)) - (speed->x * cosine) -
                  (speed->z * sine);
        Float z = (-work->speed * njSin(work->ang.y)) - (speed->z * cosine) +
                  (speed->x * sine);
        /* @hack redundant: the y term's 0, which the image adds at 0x0c905e2e;
         * without it, 39 of 151 words differ */
        Float squared = (x * x) + (0.0F * 0.0F) + (z * z);

        if (squared > THROW_SQUARED) {
            traffic_car_throw_player(player, work);
            if (squared > HURT_SQUARED) {
                work->collision->hit->owner->work->flags |= PLAYER_HURT;
            }
        }
    }
    if (player_in_range(&work->pos.point, world->model->r + PLAYER_NEAR)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
}

/* add to force[0].speed how far turning by turn, about the car, moves player
 * number. force[0] is player 0's record, and the one caller passes player 0 */
static void traffic_car_carry_turn(struct task *task, Angle turn, Sint8 number)
{
    struct task_work *player = player_work[number];
    struct car_work *work = (struct car_work *)task->work;
    struct task_force *force = task->force;
    NJS_VECTOR offset;
    NJS_VECTOR rotated;
    Float dx;
    Float dy;
    Float dz;

    offset.x = dx = player->pos.x - work->pos.point.x;
    offset.y = dy = player->pos.y - work->pos.point.y;
    offset.z = dz = player->pos.z - work->pos.point.z;
    njPushMatrix(&_unit_matrix_);
    TURN(njRotateY, turn);
    njCalcVector(NULL, &offset, &rotated);
    njPopMatrix(1);
    force[0].speed.x += rotated.x - dx;
    force[0].speed.y += rotated.y - dy;
    force[0].speed.z += rotated.z - dz;
}

/* add to force[0].speed how far player 0 moves riding the car this step. the
 * car's turn about it moves it, and so does the car's own move */
static void traffic_car_carry(struct task *task)
{
    struct task_force *force = task->force;
    struct car_work *work = (struct car_work *)task->work;
    struct task_motion *motion = task->motion;

    traffic_car_carry_turn(task, -force->turn.y, 0);
    force->speed.x += work->pos.point.x - STEP_START(motion).x;
    force->speed.y += work->pos.point.y - STEP_START(motion).y;
    force->speed.z += work->pos.point.z - STEP_START(motion).z;
}

/* set bit 0x200 of the car's flags, then clear it: the bit ends clear */
static void fun_0c906022(struct car_work *work)
{
    work->flags |= 0x200;
    work->flags &= ~0x200;
    /* @hack redundant: a never-run call of the function itself, which keeps SHC
     * from expanding it in traffic_car_run, where the image calls it by BSR at
     * 0x0c90606e; otherwise SHC emits no fun_0c906022, other code stands at
     * its offsets and 23 of traffic_car_run's 48 words differ too; without it,
     * 9 of 9 words differ */
    if (0) {
        fun_0c906022(work);
    }
}

static void traffic_car_run(struct task *task)
{
    struct car_work *work = (struct car_work *)task->work;
    struct task_motion *motion = task->motion;
    struct task_force *force = task->force;

    force->speed.x = force->speed.y = force->speed.z = 0.0F;
    STEP_START(motion) = work->pos.point;
    traffic_car_draw_advance(task);
    traffic_car_follow(task);
    traffic_car_carry(task);
    traffic_car_collide(work);
    fun_0c906022(work);
    if (work->mode == CAR_DRIVE) {
        traffic_car_drive(task);
    } else {
        traffic_car_brake(task);
    }
    collision_register((struct task_work *)work);
}

static void traffic_car_destroy(struct task *task)
{
    struct car_work *work = (struct car_work *)task->work;
    NJS_OBJECT *world = work->world;

    if (world) {
        world_remove(task, world);
        world_object_free(world);
    }
}
