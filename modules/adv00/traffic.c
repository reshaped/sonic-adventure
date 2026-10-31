/* traffic on the roads of the office and station acts */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "adv00/roads.h"
#include "adv00/texlists.h"
#include "adv00/traffic.h"
#include "camera/camera.h"
#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "effect/glare.h"
#include "object/models.h"
#include "path/path.h"
#include "player/player.h"
#include "progress/progress.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT traffic_car_object;
extern NJS_OBJECT police_car_object;
extern NJS_OBJECT taxi_object;
extern NJS_OBJECT parked_car_object;
extern NJS_MODEL traffic_car_solid_model;
extern NJS_MODEL police_car_solid_model;
extern NJS_MODEL taxi_solid_model;
extern NJS_MODEL parked_car_solid_model;
extern NJS_ACTION traffic_car_action;
extern NJS_ACTION police_car_action;
extern NJS_ACTION taxi_action;
extern NJS_ACTION parked_car_action;

/* the kinds of car, each a row of car_kinds. CAR_LIKE_PARKED is drawn with
 * the model of O ROTYU, the car parked over the way into the sewers.
 * CAR_PLAIN and CAR_LIKE_PARKED take any of car_colors */
enum car_kind { CAR_PLAIN, CAR_POLICE, CAR_TAXI, CAR_LIKE_PARKED };

struct car_kind_info {
    NJS_OBJECT *object; /* never read: car_draw draws action->object */
    NJS_MODEL *solid; /* its model in the collision world */
    Uint32 unk_08; /* never read */
    NJS_ACTION *action;
    Float unk_10; /* never read */
    Float speed; /* a car's speed as it is made, and its top speed */
};

struct car_spec {
    enum car_kind kind;
    int color; /* a row of car_colors */
    Float distance; /* along the road, where the car starts */
    Float unk_0c; /* never read */
};

/* a road and the cars that drive it */
struct traffic_group {
    enum traffic_road road;
    Uint32 unk_04; /* never read */
    int count;
    struct car_spec *cars;
};

/* the traffic task's vars. its work->mode is its row of traffic_groups */
struct traffic_vars {
    struct task *car; /* the car this step tests for earshot */
    Sint16 act; /* the act it was made in: in any other, it kills itself */
};

#define TRAFFIC(work) ((struct traffic_vars *)&(work)->vars)
#define TRAFFIC_ROAD(work) ((work)->submode) /* a row of traffic_roads */

/* in the traffic task's work->flags. each step tests one car's distance
 * from the camera, so a round, one step for each of the road's cars, tests
 * every car. TRAFFIC_NEAR means a car this round was within EARSHOT, and
 * TRAFFIC_HEARD that one was in the round before */
#define TRAFFIC_NEAR 0x400
#define TRAFFIC_HEARD 0x800

#define EARSHOT 200.0F

struct car_vars {
    Uint8 unk_00[4];
    /* the car in front: the next in its group's table, and for the last car
     * the first */
    struct task_work *ahead;
    NJS_OBJECT *solid; /* its object in the collision world */
};

#define CAR(work) ((struct car_vars *)&(work)->vars)
#define CAR_KIND(work) ((work)->tick) /* a row of car_kinds */
#define CAR_COLOR(work) ((work)->submode) /* a row of car_colors */
#define CAR_DISTANCE(work) ((work)->scl.x) /* along the road */
#define CAR_SPEED(work) ((work)->scl.y)

/* the car's middle, its model's origin, lies this far behind its position
 * on the road. its box, shadow and lights are placed from the middle */
#define CAR_MIDDLE 13.0F

/* the car heads from the road HEADING_FROM behind its position to its
 * position, turning by at most TURN_MOST a step */
#define HEADING_FROM 26.0F
#define TURN_MOST 480
/* the action frames a car plays each step for each unit of its speed */
#define ACTION_PER_SPEED 0.4F

/* car_drive's watch of the road ahead, and the speed above which a car that
 * stops for it brakes hard */
#define WATCH_RANGE 50.0F
#define WATCH_AHEAD 26.0F
#define WATCH_RADIUS 23.0F
#define BRAKE_ABOVE 1.2F

/* a car's hit sends player 0 flying FLING_SPEED along the car's ang.y less
 * FLING_TURN, and FLING_RISE up, its stick ignored for FLING_STEPS steps. it
 * then lifts player 0 by FLING_LIFT */
#define FLING_SPEED 2.7F
#define FLING_TURN 0x2000
#define FLING_RISE 2.4F
#define FLING_STEPS 60
#define FLING_LIFT 1.0F

/* a player within the car's model's radius plus this has the world test the
 * car's object */
#define WORLD_REACH 20.0F

/* where a car is on the station road, by its distance along it.
 * ROAD_FIRST_STOP runs from FIRST_STOP_AT to FIRST_CROSSING_AT, and each
 * later place from its own _AT distance to the next. ROAD_SECOND_CROSSING
 * ends at PAST_SECOND_AT, and the rest of the road is ROAD_ELSEWHERE. the
 * light holds a car at either stop. a car entering ROAD_CHANGE takes a new
 * kind, as a driving car on either road does as it starts another lap */
enum road_place {
    ROAD_ELSEWHERE,
    ROAD_FIRST_STOP,
    ROAD_FIRST_CROSSING,
    ROAD_PAST_FIRST,
    ROAD_CHANGE,
    ROAD_SECOND_STOP,
    ROAD_SECOND_CROSSING
};

#define FIRST_STOP_AT 1500.0F
#define FIRST_CROSSING_AT 1580.0F
#define PAST_FIRST_AT 1850.0F
#define CHANGE_AT 2000.0F
#define SECOND_STOP_AT 2460.0F
#define SECOND_CROSSING_AT 2560.0F
#define PAST_SECOND_AT 2840.0F

/* a car's TASK_MOTION record, as the car lays it out */
struct car_motion {
    enum road_place place;
    Uint8 unk_04[12];
    NJS_POINT3 last_pos; /* where the car stood as its step began */
    Uint8 unk_1c[8];
    Angle dip; /* its pitch about z as it brakes */
    Uint8 unk_28[16];
    Float frame; /* its frame of its kind's action */
};

#define CAR_MOTION(task) ((struct car_motion *)(task)->motion)

/* CAR_BRAKE slows the car by BRAKE_SLOWING a step to a stop, and adds
 * DIP_GAIN a step to its dip. once it stops, the dip eases back by DIP_EASE
 * a step, and the car returns to CAR_DRIVE BRAKE_WAIT + 1 steps later */
enum car_mode { CAR_DRIVE, CAR_BRAKE };

#define BRAKE_SLOWING 0.2F
#define DIP_GAIN 53
#define DIP_EASE 200
#define BRAKE_WAIT 50

/* the station road's light. LIGHT_FIRST lets the cars at the first stop go
 * and LIGHT_SECOND those at the second, each for LIGHT_TIME + 1 steps. then
 * LIGHT_CLEAR_FIRST or LIGHT_CLEAR_SECOND holds both stops until no car is
 * left in the crossing past the stop that went */
enum light { LIGHT_FIRST, LIGHT_SECOND, LIGHT_CLEAR_FIRST, LIGHT_CLEAR_SECOND };

#define LIGHT_TIME 600

/* the traffic's noise, held from the task's start until FLAG_TRAFFIC is
 * cleared or the task is destroyed */
#define SE_TRAFFIC 0x41f
/* the engines, which sound each step while TRAFFIC_HEARD is set */
#define SE_ENGINE 0x21c
#define SE_STOP 0x21d /* a car braking hard */
#define SE_HIT 0x21e /* a car sending player 0 flying */

static void traffic_init(struct task *task);
static void traffic_step(struct task *task);
static void traffic_destroy(struct task *task);
static void traffic_create_cars(struct task *task);
static void car_init(struct task *task);
static void car_draw(struct task *task);
static void car_step(struct task *task);
static void car_destroy(struct task *task);
static void car_pick_kind(struct task_work *work);

/* the kinds' actions in car_kind's order but for the last two: the parked
 * car's comes ahead of the taxi's. no code reads the table */
static NJS_ACTION *car_actions[4] = {
    &traffic_car_action,
    &police_car_action,
    &parked_car_action,
    &taxi_action,
};

/* the textures of sscar_texlist a car's color picks: red, white, light
 * blue, black, yellow and chrome */
static Uint32 car_colors[6] = {9, 11, 8, 6, 12, 7};

static struct path *traffic_roads[2] = {&office_road, &station_road};

/* clang-format off */
static struct collision_shape car_shapes[4] = {
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {CAR_MIDDLE, 3.5F, 0.0F}, 20.0F, 3.5F, 7.5F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {CAR_MIDDLE, 3.5F, 0.0F}, 20.0F, 3.5F, 7.5F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {CAR_MIDDLE, 3.5F, 0.0F}, 20.0F, 3.5F, 7.5F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {CAR_MIDDLE, 3.5F, 0.0F}, 20.0F, 3.5F, 7.5F, {0}, 0, NULL},
};
/* clang-format on */

static struct car_kind_info car_kinds[4] = {
    {&traffic_car_object, &traffic_car_solid_model, 0, &traffic_car_action,
     0.038F, 2.4F},
    {&police_car_object, &police_car_solid_model, 0, &police_car_action, 0.03F,
     2.0F},
    {&taxi_object, &taxi_solid_model, 0, &taxi_action, 0.05F, 3.5F},
    {&parked_car_object, &parked_car_solid_model, 0, &parked_car_action, 0.038F,
     2.4F},
};

static struct car_spec office_cars[4] = {
    {CAR_PLAIN, 0, 0.0F, 90.0F},
    {CAR_POLICE, 0, 700.0F, 90.0F},
    {CAR_TAXI, 0, 1400.0F, 90.0F},
    {CAR_LIKE_PARKED, 0, 2100.0F, 90.0F},
};
static struct car_spec station_cars[6] = {
    {CAR_PLAIN, 0, 0.0F, 90.0F},    {CAR_PLAIN, 1, 700.0F, 90.0F},
    {CAR_PLAIN, 2, 1400.0F, 90.0F}, {CAR_PLAIN, 3, 2100.0F, 90.0F},
    {CAR_PLAIN, 4, 2800.0F, 90.0F}, {CAR_PLAIN, 5, 3500.0F, 90.0F},
};

static struct traffic_group traffic_groups[2] = {
    {TRAFFIC_OFFICE, 0, 4, office_cars},
    {TRAFFIC_STATION, 0, 6, station_cars},
};

/* four values that no code reads */
static Float dat_0cab3260[4] = {1100.0F, 1168.0F, 3408.0F, 3928.0F};

/* the distances ahead at which car_drive samples the road for a bend */
static Float car_lookaheads[3] = {65.0F, 90.0F, 115.0F};

/* the station road's light, and the cars in each of its crossings */
static int first_crossing_cars;
static int second_crossing_cars;
static enum light traffic_light;
static int light_time; /* the steps LIGHT_FIRST or LIGHT_SECOND has held */
#ifdef GCC
/* where the car's first search of its road this step stopped, which its
 * later searches start from. car_step empties it */
static struct path_mark car_mark;
#endif

/* TRUE once the story of player 0's character lets the cars drive */
static Bool traffic_open(void)
{
    switch (player_get_character(0)) {
    case CHARACTER_SONIC:
        return event_flags[FLAG_SONIC_SS_BARRICADE];
    case CHARACTER_TAILS:
        return event_flags[FLAG_TAILS_SS_BARRICADE];
    case CHARACTER_KNUCKLES:
    case CHARACTER_AMY:
    case CHARACTER_E102:
    case CHARACTER_BIG:
        return TRUE;
    }
    return FALSE;
}

struct task *traffic_start(int road)
{
    struct task *task;

    if (traffic_open()) {
        task = task_create(TASK_WORK, 3, traffic_init);
        if (task) {
            task->work->mode = road % 2;
        }
    } else {
        task = NULL;
    }
    police_car_object.model->mats[5].attrflags &= ~NJD_FLAG_IGNORE_LIGHT;
    return task;
}

static void traffic_init(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task)) {
        traffic_create_cars(task);
    }
    TRAFFIC(work)->act = stage_get_stage_act();
    TRAFFIC_ROAD(work) = traffic_groups[work->mode].road;
    first_crossing_cars = 0;
    second_crossing_cars = 0;
    traffic_light = LIGHT_CLEAR_FIRST;
    light_time = 0;
    work->flags = 0;
    work->timer = 0;
    TRAFFIC(work)->car = task->child;
    event_flags[FLAG_TRAFFIC] = TRUE;
    sound_play_held(SE_TRAFFIC, -1, 8, 0);
    task->step = traffic_step;
    task->destructor = traffic_destroy;
}

static void traffic_step(struct task *task)
{
    struct task_work *work = task->work;
    int count;
    struct task *car;
    char *car_pos;
    char *camera_pos;
    Float x;
    Float y;
    Float z;
    Float distance;
    Float camera_x;
    Float camera_y;
    Float camera_z;

    if (stage_get_stage_act() != TRAFFIC(work)->act) {
        task_kill(task);
        return;
    }
    if (!event_flags[FLAG_TRAFFIC]) {
        sound_stop(SE_TRAFFIC);
        return;
    }
    if (TRAFFIC_ROAD(work) == TRAFFIC_STATION) {
        switch (traffic_light) {
        case LIGHT_FIRST:
            light_time++;
            if (light_time > LIGHT_TIME) {
                light_time = 0;
                traffic_light = LIGHT_CLEAR_FIRST;
            }
            break;
        case LIGHT_CLEAR_FIRST:
            if (!first_crossing_cars) {
                light_time = 0;
                traffic_light = LIGHT_SECOND;
            }
            break;
        case LIGHT_SECOND:
            light_time++;
            if (light_time > LIGHT_TIME) {
                light_time = 0;
                traffic_light = LIGHT_CLEAR_SECOND;
            }
            break;
        case LIGHT_CLEAR_SECOND:
            if (!second_crossing_cars) {
                light_time = 0;
                traffic_light = LIGHT_FIRST;
            }
            break;
        }
    }
    count = traffic_groups[work->mode].count;
    car = TRAFFIC(work)->car;
    /* @hack type: the car's and the camera's pos read a Float at a time
     * through byte cursors, each stepped as a Uint32; stepped with +=, 4 of
     * 177 words differ, and read through the members, 55 */
    car_pos = (char *)&car->work->pos;
    camera_pos = (char *)&camera_work->pos;
    camera_x = *(Float *)camera_pos;
    x = *(Float *)car_pos;
    car_pos = (char *)((Uint32)car_pos + sizeof(Float));
    camera_pos = (char *)((Uint32)camera_pos + sizeof(Float));
    camera_y = *(Float *)camera_pos;
    y = *(Float *)car_pos;
    x -= camera_x;
    car_pos = (char *)((Uint32)car_pos + sizeof(Float));
    camera_pos = (char *)((Uint32)camera_pos + sizeof(Float));
    camera_z = *(Float *)camera_pos;
    y -= camera_y;
    z = *(Float *)car_pos;
    z -= camera_z;
    distance = (x * x) + (y * y) + (z * z);
    if (distance < EARSHOT * EARSHOT) {
        work->flags |= TRAFFIC_NEAR;
    }
    TASK_TIMER(work)++;
    if (TASK_TIMER(work) >= count) {
        if (work->flags & TRAFFIC_NEAR) {
            work->flags |= TRAFFIC_HEARD;
            TASK_TIMER(work) = 0;
        } else {
            work->flags &= ~TRAFFIC_HEARD;
            TASK_TIMER(work) = 0;
        }
        work->flags &= ~TRAFFIC_NEAR;
        TRAFFIC(work)->car = task->child;
    } else {
        TRAFFIC(work)->car = car->next;
    }
    if (work->flags & TRAFFIC_HEARD) {
        sound_play_for(SE_ENGINE, (int)task, 1, 0, 2);
    }
    if (!placement_editing(task)) {
        task_step_children(task);
    }
}

static void traffic_destroy(struct task *task)
{
    /* @hack spelling: a copy of task, which keeps task in R14 across
     * placement_editing (MOV R4,R14 at 0x0c907826); without it, task goes
     * to sp+0 and 14 of 15 words differ */
    struct task *self = task;

    if (!placement_editing(self)) {
        task_destroy_children(self);
    }
    sound_stop(SE_TRAFFIC);
}

/* make the group's cars last to first, each its task's first child, so
 * the children run in table order. each car's ahead is the next car in the
 * table, and the last car's is the first */
static void traffic_create_cars(struct task *task)
{
    struct task_work *work = task->work;
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    int mode = work->mode;
    int count = traffic_groups[mode].count;
    struct car_spec *specs;
    struct car_spec *spec;
    struct task *car;
    struct task *last;
    struct task_work *car_work;
    int i;
    int kind;

    for (specs = traffic_groups[mode].cars, spec = &specs[i = count - 1];
         i >= 0; i--, spec--) {
        car = task_create_child(TASK_WORK | TASK_MOTION | TASK_FORCE, car_init,
                                task);
        if (car) {
            car_work = car->work;
            kind = spec->kind;
            car_work->mode = CAR_DRIVE;
            CAR_COLOR(car_work) = spec->color;
            CAR_KIND(car_work) = kind;
            car_work->timer = 0;
            CAR_DISTANCE(car_work) = spec->distance;
            CAR_SPEED(car_work) = car_kinds[kind].speed;
            if (car->next) {
                CAR(car_work)->ahead = car->next->work;
            }
        }
    }
    last = task->child;
    while (last->next) {
        last = last->next;
    }
    CAR(last->work)->ahead = car->work;
}

static void car_init(struct task *task)
{
    struct task_work *work = task->work;
    struct car_motion *motion = CAR_MOTION(task);
    NJS_OBJECT *solid;
    Float distance;

    CAR(work)->solid = solid = world_object_alloc();
    solid->evalflags = NJD_EVAL_BREAK;
    solid->scl[0] = 1.0F;
    solid->scl[1] = 1.0F;
    solid->scl[2] = 1.0F;
    solid->ang[0] = work->ang.x;
    solid->ang[1] = work->ang.y;
    solid->ang[2] = work->ang.z;
    solid->pos[0] = work->pos.x;
    solid->pos[1] = work->pos.y;
    solid->pos[2] = work->pos.z;
    solid->model = car_kinds[CAR_KIND(work)].solid;
    solid->child = NULL;
    solid->sibling = NULL;
    world_add(WORLD_MOVES, task, solid);
    collision_init(task, &car_shapes[CAR_KIND(work)], COLLISION_OBJECT, 1, 1);
    motion->dip = 0;
    switch (TRAFFIC_ROAD(task->parent->work)) {
    case TRAFFIC_STATION:
        distance = CAR_DISTANCE(work);
        if (distance >= FIRST_STOP_AT && distance <= PAST_SECOND_AT) {
            if (distance < FIRST_CROSSING_AT) {
                motion->place = ROAD_FIRST_STOP;
            } else if (distance < PAST_FIRST_AT) {
                motion->place = ROAD_FIRST_CROSSING;
                first_crossing_cars++;
            } else if (distance < CHANGE_AT) {
                motion->place = ROAD_PAST_FIRST;
            } else if (distance < SECOND_STOP_AT) {
                motion->place = ROAD_CHANGE;
            } else if (distance < SECOND_CROSSING_AT) {
                motion->place = ROAD_SECOND_STOP;
            } else {
                motion->place = ROAD_SECOND_CROSSING;
                second_crossing_cars++;
            }
        } else {
            motion->place = ROAD_ELSEWHERE;
            /* @hack redundant: a break that repeats the case's, which lets this
             * store share the default's (MOV.L R11,@R13 at 0x0c907a7c);
             * without it, each has its own and 17 of 145 words differ */
            break;
        }
        break;
    default:
        motion->place = ROAD_ELSEWHERE;
    }
    task->destructor = car_destroy;
    task->draw = car_draw;
    task->step = car_step;
}

/* put the car on the road at its distance, and head it as HEADING_FROM
 * says. tilt it as the road is at its middle. on the station road, track the
 * car's place too */
static void car_follow_road(struct task *task)
{
    struct task_work *work = task->work;
    struct car_motion *motion = CAR_MOTION(task);
    struct task_force *force = task->force;
    struct path *road = traffic_roads[TRAFFIC_ROAD(task->parent->work)];
    struct path_sample samples[3];
    Float distances[3];
    Float length = road->length;
    Float distance = CAR_DISTANCE(work);
    Float turn;
    Float dx;
    Float dz;
    struct path_sample *here;
    Float frame;
    Float last_frame;
    Angle ang_y;
    Angle old_y;
    Angle turned;
    int i;

    distances[0] = distance - HEADING_FROM;
    distances[1] = distance;
    distances[2] = distance - CAR_MIDDLE;
    for (i = 0; i < 3; ++i) {
        if (distances[i] > length) {
            distances[i] -= length;
        } else if (distances[i] < 0.0F) {
            distances[i] += length;
        }
        samples[i].distance = distances[i];
#ifdef GCC
        path_sample_from(road, &samples[i], &car_mark);
#else
        path_sample(road, &samples[i]);
#endif
    }
    here = &samples[1];
    dx = samples[1].pos_copy.x - samples[0].pos_copy.x;
    dz = samples[1].pos_copy.z - samples[0].pos_copy.z;
    ang_y =
        ((Angle)(atan2f(dx, dz) * (Float)ANGLE_PER_RADIAN) + 0x4000) & 0xffff;
    turn = ang_y - work->ang.y;
    if (turn > 32768.0F) {
        turn += -65536.0F;
    } else if (turn < -32768.0F) {
        turn += 65536.0F;
    }
    if (turn > TURN_MOST) {
        ang_y = work->ang.y + TURN_MOST;
    } else if (turn < -TURN_MOST) {
        ang_y = work->ang.y - TURN_MOST;
    }
    old_y = work->ang.y;
    work->ang.x = samples[2].smooth_x;
    work->ang.y = ang_y;
    work->ang.z = samples[2].smooth_z;
    turned = angle_difference(work->ang.y, old_y);
    force->turn.y = turned;
    work->pos.x = here->pos_copy.x;
    work->pos.y = here->pos_copy.y;
    work->pos.z = here->pos_copy.z;
    if (TRAFFIC_ROAD(task->parent->work) == TRAFFIC_STATION) {
        switch (motion->place) {
        case ROAD_ELSEWHERE:
            if (distance > FIRST_STOP_AT && distance < FIRST_CROSSING_AT) {
                motion->place = ROAD_FIRST_STOP;
            }
            break;
        case ROAD_FIRST_STOP:
            if (distance > FIRST_CROSSING_AT) {
                motion->place = ROAD_FIRST_CROSSING;
                ++first_crossing_cars;
            }
            break;
        case ROAD_FIRST_CROSSING:
            if (distance > PAST_FIRST_AT) {
                motion->place = ROAD_PAST_FIRST;
                --first_crossing_cars;
            }
            break;
        case ROAD_PAST_FIRST:
            if (distance > CHANGE_AT) {
                motion->place = ROAD_CHANGE;
                car_pick_kind(work);
            }
            break;
        case ROAD_CHANGE:
            if (distance > SECOND_STOP_AT) {
                motion->place = ROAD_SECOND_STOP;
            }
            break;
        case ROAD_SECOND_STOP:
            if (distance > SECOND_CROSSING_AT) {
                motion->place = ROAD_SECOND_CROSSING;
                ++second_crossing_cars;
            }
            break;
        case ROAD_SECOND_CROSSING:
            if (distance > PAST_SECOND_AT) {
                motion->place = ROAD_ELSEWHERE;
                --second_crossing_cars;
            }
            break;
        }
    }
    frame = motion->frame;
    last_frame = (Float)(car_kinds[CAR_KIND(work)].action->motion->nbFrame - 1);
    frame += CAR_SPEED(work) * ACTION_PER_SPEED;
    if (frame >= last_frame) {
        frame -= last_frame;
    }
    motion->frame = frame;
}

/* drive the car toward an aim speed, no more than its kind's top speed. the
 * aim is lower in a bend or behind a slower car, and 0 at a stop the light
 * holds. while a player is within WATCH_RANGE of the car, the car watches
 * the road WATCH_AHEAD ahead. a player within WATCH_RADIUS of that point, or
 * anything touching the car, then makes the aim 0. a car that stops so from
 * above BRAKE_ABOVE starts CAR_BRAKE. a car that starts another lap takes a
 * new kind */
static void car_drive(struct task *task)
{
    struct task_work *work = task->work;
    struct car_motion *motion = CAR_MOTION(task);
    struct car_motion *motion_copy;
    struct task_work *ahead;
    struct path *road;
    Float speed;
    Float distance;
    Float gap;
    Float length;
    Float most;
    Float aim;
    Float bend;
    Float accel;
    Float along;
    Float speed_copy;
    int i;

    ahead = CAR(work)->ahead;
    distance = CAR_DISTANCE(work);
    gap = CAR_DISTANCE(ahead) - distance;
    speed = CAR_SPEED(work);
    road = traffic_roads[TRAFFIC_ROAD(task->parent->work)];
    length = road->length;
    aim = car_kinds[CAR_KIND(work)].speed;
    most = aim;
    /* @hack redundant: a never-run test of ahead, which keeps task in R9,
     * ahead in R10 and path_sample's address in R8 (0x0c907e3c, 0x0c907e4c
     * and 0x0c907e8c); without it, the three trade and 12 of 315 words
     * differ */
    if (0) {
        if (ahead) {
        }
    }
    if (gap < 0.0F) {
        gap += length;
    }
    {
        struct path_sample samples[3];
        NJS_VECTOR first;
        NJS_VECTOR second;

        for (i = 0; i < 3; i++) {
            along = car_lookaheads[i] + distance;
            if (along >= length) {
                along -= length;
            }
            samples[i].distance = along;
#ifdef GCC
            path_sample_from(road, &samples[i], &car_mark);
#else
            path_sample(road, &samples[i]);
#endif
        }
        first.x = samples[1].pos_copy.x - samples[0].pos_copy.x;
        first.y = 0.0F;
        first.z = samples[1].pos_copy.z - samples[0].pos_copy.z;
        second.x = samples[2].pos_copy.x - samples[0].pos_copy.x;
        second.y = 0.0F;
        second.z = samples[2].pos_copy.z - samples[0].pos_copy.z;
        njUnitVector(&first);
        njUnitVector(&second);
        bend = njInnerProduct(&first, &second);
    }
    if (bend <= 0.995F) {
        aim = 0.8F;
    } else if (bend <= 0.996F) {
        aim = 1.2F;
    } else if (bend <= 0.998F) {
        aim = 1.6F;
    }
    /* @hack spelling: a copy of speed, which puts speed at sp+0, distance at
     * sp+4 and motion at sp+8 in the stack frame (0x0c907e46 to
     * 0x0c907e5a); without it, 166 of 315 words differ */
    speed_copy = speed;
    if (gap < (speed_copy * 80.0F) + 55.0F && aim > CAR_SPEED(ahead)) {
        aim = CAR_SPEED(ahead) - (6.0F / gap);
    }
    /* @hack spelling: a copy of motion, which the image keeps at sp+8 and
     * reads for each stop's test (MOV.L @(8,R15),R0 at 0x0c907fc8 and
     * 0x0c908000); without it, car_drive is 32 bytes longer and 280 of 315
     * words differ */
    motion_copy = motion;
    if (TRAFFIC_ROAD(task->parent->work) == TRAFFIC_STATION) {
        if (motion_copy->place == ROAD_FIRST_STOP &&
            traffic_light != LIGHT_FIRST) {
            aim = 0.0F;
        } else if (motion_copy->place == ROAD_SECOND_STOP &&
                   traffic_light != LIGHT_SECOND) {
            aim = 0.0F;
        }
    }
    if (player_in_range(&work->pos, WATCH_RANGE)) {
        struct path_sample front;

        along = distance + WATCH_AHEAD;
        if (along >= length) {
            along -= length;
        }
        front.distance = along;
#ifdef GCC
        path_sample_from(road, &front, &car_mark);
#else
        path_sample(road, &front);
#endif
        if (player_in_range(&front.pos_copy, WATCH_RADIUS) ||
            (work->flags & COLLISION_TOUCHING)) {
            aim = 0.0F;
            if (speed > BRAKE_ABOVE) {
                work->mode = CAR_BRAKE;
                sound_play(SE_STOP, 0, 0, 0);
            }
        }
    }
    accel = (aim - speed) * 0.01F;
    if (accel < 0.0F) {
        accel *= 4.0F;
    }
    if (accel > 0.08F) {
        accel = 0.08F;
    } else if (accel < -0.9F) {
        accel = -0.9F;
    }
    CAR_SPEED(work) += accel;
    if (CAR_SPEED(work) > most) {
        CAR_SPEED(work) = most;
    } else if (CAR_SPEED(work) < 0.0F) {
        CAR_SPEED(work) = 0.0F;
    }
    CAR_DISTANCE(work) += CAR_SPEED(work);
    if (CAR_DISTANCE(work) > length) {
        CAR_DISTANCE(work) -= length;
        car_pick_kind(work);
    }
}

/* give the car a random kind, a random color if the kind takes one, and
 * the kind's model in the collision world */
static void car_pick_kind(struct task_work *work)
{
    Float pick = rand() * RAND_SCALE;
    int kind;
    int color;

    if (pick < 0.4F) {
        kind = CAR_PLAIN;
    } else if (pick < 0.8F) {
        kind = CAR_LIKE_PARKED;
    } else if (pick < 0.95F) {
        kind = CAR_TAXI;
    } else {
        kind = CAR_POLICE;
    }
    if (kind == CAR_PLAIN || kind == CAR_LIKE_PARKED) {
        color = (int)(rand() * (6 * RAND_SCALE));
    } else {
        color = 0;
    }
    CAR_KIND(work) = kind;
    CAR_COLOR(work) = color;
    /* @hack spelling: the solid read into a local in a bare scope, which
     * loads it into R4 and the model into R2 (0x0c9081f2 and 0x0c9081f4);
     * without it, R3 and R1 take them and 5 of 67 words differ */
    {
        NJS_OBJECT *solid = CAR(work)->solid;

        solid->model = car_kinds[kind].solid;
    }
}

static void car_brake(struct task *task)
{
    struct task_work *work = task->work;
    struct car_motion *motion = CAR_MOTION(task);
    struct path *road = traffic_roads[TRAFFIC_ROAD(task->parent->work)];

    if (CAR_SPEED(work) != 0.0F) {
        CAR_SPEED(work) -= BRAKE_SLOWING;
        motion->dip += DIP_GAIN;
        if (CAR_SPEED(work) < 0.0F) {
            CAR_SPEED(work) = 0.0F;
            work->timer = 0;
        }
    } else {
        if (motion->dip > 0) {
            motion->dip -= DIP_EASE;
            if (motion->dip < 0) {
                motion->dip = 0;
            }
        }
        ++TASK_TIMER(work);
        if (TASK_TIMER(work) > BRAKE_WAIT) {
            work->timer = 0;
            work->mode = CAR_DRIVE;
        }
    }
    CAR_DISTANCE(work) += CAR_SPEED(work);
    if (CAR_DISTANCE(work) > road->length) {
        CAR_DISTANCE(work) -= road->length;
    }
}

/* each kind's two headlights and two tail lights, as x, y and z from its
 * middle. a car's front is toward -x */
static const Float car_headlights[4][6] = {
#include "adv00/traffic/car_headlights.inc"
};
static const Float car_tail_lights[4][6] = {
#include "adv00/traffic/car_tail_lights.inc"
};

/* draw the headlights' glare when lit is TRUE, and the tail lights' while
 * the car brakes */
static void car_draw_lights(struct task_work *work, Bool lit)
{
    NJS_POINT3 lights[4];
    NJS_POINT3 point;
    NJS_VECTOR front_way = {-1.0F, 0.0F, 0.0F};
    NJS_VECTOR front;
    NJS_VECTOR back_way = {1.0F, 0.0F, 0.0F};
    NJS_VECTOR back;
    NJS_ARGB front_color = {0.3F, 1.0F, 1.0F, 0.95F};
    NJS_ARGB back_color = {0.3F, 1.0F, 0.2F, 0.0F};
    int kind;
    Angle ang;
    Float z;
    Float y;
    Float x;
    Float *pos;
    const Float *table;
    int i;

    kind = CAR_KIND(work);
    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    /* @hack spelling: the position read a Float at a time through a cursor,
     * which loads it by FMOV.S @R4+,FR13 from 0x0c90839e; read through the
     * members, 259 of 349 words differ */
    pos = &work->pos.x;
    x = *pos++;
    y = *pos++;
    z = *pos;
    /* @hack spelling: lit compared with TRUE, CMP/EQ #1 at 0x0c9083b8. both
     * calls pass TRUE or FALSE, so if (lit) is the same test; as that, 16 of
     * 349 words differ */
    if (lit == TRUE) {
        njCalcVector(NULL, &front_way, &front);
        njUnitVector(&front);
        table = car_headlights[kind];
        for (i = 0; i < 2; ++i) {
            point.x = *table++ + CAR_MIDDLE;
            point.y = *table++;
            point.z = *table++;
            njCalcPoint(NULL, &point, &lights[i]);
            lights[i].x += x;
            lights[i].y += y;
            lights[i].z += z;
        }
    }
    if (work->mode == CAR_BRAKE) {
        njCalcVector(NULL, &back_way, &back);
        njUnitVector(&back);
        table = car_tail_lights[kind];
        for (i = 2; i < 4; ++i) {
            point.x = *table++ + CAR_MIDDLE;
            point.y = *table++;
            point.z = *table++;
            njCalcPoint(NULL, &point, &lights[i]);
            lights[i].x += x;
            lights[i].y += y;
            lights[i].z += z;
        }
    }
    njPopMatrix(1);
    /* @hack spelling: lit compared with TRUE again, CMP/EQ #1 at 0x0c908578.
     * both calls pass TRUE or FALSE, so if (lit) is the same test; as that, 3
     * of 349 words differ */
    if (lit == TRUE) {
        glare_draw(&lights[0], &front, 1.6F, &front_color);
        glare_draw(&lights[1], &front, 1.6F, &front_color);
    }
    if (work->mode == CAR_BRAKE) {
        glare_draw(&lights[2], &back, 0.4F, &back_color);
        glare_draw(&lights[3], &back, 0.4F, &back_color);
    }
}

static void car_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct car_motion *motion = CAR_MOTION(task);
    int kind = CAR_KIND(work);
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    int color = CAR_COLOR(work);
    Float far_clip = _nj_far_clip_;
    Angle ang;
    NJS_ACTION *action;
    NJS_OBJECT *object;

    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 30.0F)) {
        return;
    }
    if (njDistanceP2P(&work->pos, &camera_work->pos) > far_clip) {
        return;
    }
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    texture_set_regular();
    njPushMatrix(NULL);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njTranslate(NULL, CAR_MIDDLE, 0.2F, 0.0F);
    njScale(NULL, 5.0F, 1.0F, 2.5F);
    draw_tree(shadow_object);
    njPopMatrix(1);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    njTranslate(NULL, CAR_MIDDLE, 0.0F, 0.0F);
    if (work->mode == CAR_BRAKE) {
        TURN_VIA(njRotateZ, motion->dip, ang);
    }
    njSetTexture(&sscar_texlist);
    action = car_kinds[kind].action;
    object = action->object;
    switch (kind) {
    /* @hack redundant: CAR_PLAIN's case in a bare scope, which gives R1 the
     * model and R2 its materials in CAR_LIKE_PARKED's case (MOV.L
     * @(4,R6),R1 at 0x0c90876a); without it, the two trade and 5 of 206
     * words differ */
    case CAR_PLAIN: {
        object->model->mats[0].attr_texId = car_colors[color];
        break;
    }
    case CAR_POLICE:
        break;
    case CAR_TAXI:
        /* @hack redundant: a read of the color nothing uses, which leaves R1
         * the model in CAR_LIKE_PARKED's case (MOV.L @(4,R6),R1 at
         * 0x0c90876a); without it, R3 takes it and 4 of 206 words differ */
        (void)car_colors[color];
        break;
    case CAR_LIKE_PARKED:
        object->model->mats[2].attr_texId = car_colors[color];
        break;
    }
    draw_action(action, motion->frame);
    njPopMatrix(1);
    switch (story_get_time()) {
    case TIME_EVENING:
    case TIME_NIGHT:
        car_draw_lights(work, TRUE);
        break;
    case TIME_DAY:
        car_draw_lights(work, FALSE);
        break;
    }
}

/* send player 0 flying from the car. it faces the fling's heading or the
 * opposite, whichever is nearer its own facing, and flies the same way
 * either way */
static void car_hit_player(struct task_work *work)
{
    struct task_work *player = player_work[0];
    NJS_VECTOR speed;
    Angle ang[3];

    ang[0] = 0;
    ang[1] = work->ang.y - FLING_TURN;
    ang[2] = 0;
    if (angle_distance(ang[1], player->ang.y) > 0x4000) {
        ang[1] += 0x8000;
        speed.x = -FLING_SPEED;
    } else {
        speed.x = FLING_SPEED;
    }
    speed.y = FLING_RISE;
    speed.z = 0.0F;
    player_send_flying(0, &speed, ang, FLING_STEPS);
    player->pos.y += FLING_LIFT;
    sound_play(SE_HIT, 0, 0, 0);
}

/* put the car's object in the collision world where the car stands. the
 * world tests it while a player is within its model's radius plus
 * WORLD_REACH */
static void car_collide(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *solid = CAR(work)->solid;
    struct task_work *player = player_work[0];
    struct task_motion *motion = player_motion[0];
    NJS_VECTOR closing;
    Float closing2;

    solid->pos[0] = work->pos.x;
    solid->pos[1] = work->pos.y;
    solid->pos[2] = work->pos.z;
    solid->ang[0] = work->ang.x;
    solid->ang[1] = work->ang.y;
    solid->ang[2] = work->ang.z;
    closing.x = (CAR_SPEED(work) * njCos(work->ang.y)) -
                (motion->speed.x * njCos(player->ang.y)) -
                (motion->speed.z * njSin(player->ang.y));
    closing.y = 0.0F;
    closing.z = (-CAR_SPEED(work) * njSin(work->ang.y)) +
                (motion->speed.x * njSin(player->ang.y)) -
                (motion->speed.z * njCos(player->ang.y));
    closing2 = (closing.x * closing.x) + (closing.y * closing.y) +
               (closing.z * closing.z);
    /* @bug logic: a touch sends player 0 flying only from the car's shape
     * 1 (CMP/EQ #1,R0 at 0x0c908a0a), but collision_init gives a car one
     * shape, shape 0, so no car sends a player flying */
    if (CAR_SPEED(work) > 1.0F && (work->flags & COLLISION_TOUCHING) &&
        work->collision->own_shape == 1 &&
        work->collision->hit->kind == COLLISION_PLAYER && closing2 > 1.7F) {
        car_hit_player(work);
    }
    if (player_in_range(&work->pos, solid->model->r + WORLD_REACH)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
}

/* add to the force's speed the move that turning by turn about the car
 * gives the player's position */
static void car_carry_turn(struct task *task, Angle turn, int player)
{
    struct task_work *work;
    struct task_force *force;
    struct task_work *rider;
    NJS_VECTOR offset;
    NJS_VECTOR turned;
    NJS_VECTOR before;

    rider = player_work[player];
    work = task->work;
    force = task->force;
    offset.x = before.x = rider->pos.x - work->pos.x;
    offset.y = before.y = rider->pos.y - work->pos.y;
    offset.z = before.z = rider->pos.z - work->pos.z;
    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    TURN(njRotateY, turn);
    njCalcVector(NULL, &offset, &turned);
    njPopMatrix(1);
    force->speed.x += turned.x - before.x;
    force->speed.y += turned.y - before.y;
    force->speed.z += turned.z - before.z;
}

/* give the force's speed the car's move this step, and the move its turn
 * gives player 0 */
static void car_carry(struct task *task)
{
    struct task_force *force = task->force;
    struct task_work *work = task->work;
    struct car_motion *motion = CAR_MOTION(task);

    car_carry_turn(task, -force->turn.y, 0);
    force->speed.x += work->pos.x - motion->last_pos.x;
    force->speed.y += work->pos.y - motion->last_pos.y;
    force->speed.z += work->pos.z - motion->last_pos.z;
}

/* draw the car where it stood, then move it */
static void car_step(struct task *task)
{
    struct task_work *work;
    struct car_motion *motion;
    struct task_force *force;

    work = task->work;
    motion = CAR_MOTION(task);
    force = task->force;
    force->speed.x = 0.0F;
    force->speed.y = 0.0F;
    force->speed.z = 0.0F;
    motion->last_pos.x = work->pos.x;
    motion->last_pos.y = work->pos.y;
    motion->last_pos.z = work->pos.z;
    car_draw(task);
#ifdef GCC
    car_mark.index = -1;
#endif
    car_follow_road(task);
    car_carry(task);
    car_collide(task);
    if (work->mode == CAR_DRIVE) {
        car_drive(task);
    } else {
        car_brake(task);
    }
    collision_register(work);
}

static void car_destroy(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *solid = CAR(work)->solid;

    if (solid) {
        world_remove(task, solid);
        world_object_free(solid);
    }
}
