/* O MONORAIL two-car train that the deck's station calls to carry player 0 into
 * the Egg Carrier */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv0100/monorail.h"
#include "adv0100/station.h"
#include "collision/collision.h"
#include "collision/world.h"
#include "draw/tree.h"
#include "player/player.h"
#include "progress/progress.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/texture.h"

/* the inside objects are the cars' floors. a car's first child and that
 * child's own child are the two parts of its door */
extern NJS_MOTION monorail_route_0;
extern NJS_MOTION monorail_route_1;
extern NJS_OBJECT monorail_front_object;
extern NJS_OBJECT monorail_front_solid_object;
extern NJS_OBJECT monorail_front_inside_object;
extern NJS_OBJECT monorail_rear_object;
extern NJS_OBJECT monorail_rear_solid_object;
extern NJS_OBJECT monorail_rear_inside_object;

/* the distance along the route at which the train stops, and how far short of
 * the stop it starts to slow */
#define STOP_DISTANCE 480.0F
#define SLOW_DISTANCE 200.0F
/* the rear car's offset along the route from the front's */
#define REAR_OFFSET (-84.0F)

/* the steps the train stands at the stop before its doors open, and those it
 * waits once they start to shut before it tests them */
#define STOP_STEPS 60
/* the steps the train runs with a passenger before it sends player 0 inside */
#define LEAVE_STEPS 60

/* distance within which a player must be for the collision world to test a
 * car */
#define CAR_RANGE 100.0F

/* in a car's work->flags: the train's doors stand fully open, as
 * monorail_car_mark last found them */
#define CAR_OPEN 0x200

/* the flags of a car's copy and of an inside floor's in the collision world */
#define CAR_FLAGS (WORLD_MEDIUM | WORLD_MOVES | WORLD_UNCLIMBABLE | WORLD_SOLID)
#define INSIDE_FLAGS (WORLD_MEDIUM | WORLD_SOLID)

/* how far a car's door turns about x when fully open, a sixth of a turn. its
 * child part then turns DOOR_CHILD_TURN within it, three sixteenths */
#define DOOR_TURN 10922.0F
#define DOOR_CHILD_TURN 12288.0F

/* the train coming in, and its doors opening */
#define SE_MONORAIL_COMES 563
#define SE_MONORAIL_DOORS 562

/* both players' pads, as the image passes them: pad_disable_player takes any
 * number but 0 and 1 for both, and PAD_BOTH_PLAYERS is 0xff */
#define BOTH_PADS 2

/* where the train takes player 0: the Egg Carrier inside's act, and the
 * entrance in it */
#define INSIDE_ACT 1
#define INSIDE_ENTRANCE 2

/* how far open the doors stand where their fast opening or shutting ends */
#define DOORS_AJAR 0.2F
/* door_steps at the last step of DOORS_START_OPENING */
#define DOORS_START_STEPS 20

/* how the doors move, in struct monorail's doors */
enum monorail_doors {
    DOORS_OPEN,
    DOORS_SHUT,
    /* train->open climbs fast to DOORS_AJAR and holds there. the step that
     * finds door_steps at DOORS_START_STEPS, counting from 0, gives way to
     * DOORS_OPENING */
    DOORS_START_OPENING,
    DOORS_OPENING, /* train->open climbs slowly the rest of the way to 1 */
    DOORS_START_SHUTTING, /* train->open falls fast to DOORS_AJAR */
    DOORS_SHUTTING /* train->open falls slowly the rest of the way to 0 */
};

/* the train's record, allocated from the heap at task->motion, which the cars
 * read too */
struct monorail {
    Uint8 unk_00[2];
    Sint8 doors; /* a monorail_doors value */
    Uint8 unk_03;
    Sint16 door_steps; /* steps spent in DOORS_START_OPENING */
    Uint8 unk_06[2];
    int leave_steps; /* steps the train has run with a passenger */
    Float distance; /* the front car's distance along the route */
    Float speed;
    Uint8 unk_14[4];
    Float open; /* how far the doors are open, from 0 to 1 */
    Float stop; /* distance along the route at which the train stops */
    Uint8 unk_20[4];
    NJS_MOTION *route;
    Float *lengths; /* distance from each frame of the route to the next */
    Float length; /* the lengths' sum */
    int count; /* number of lengths */
    struct task_work *front;
    struct task_work *rear;
};

#define MONORAIL(task) ((struct monorail *)(task)->motion)

/* the vars of a car and of its inside floor */
struct car_vars {
    NJS_OBJECT *solid; /* its copy in the collision world */
    /* the tree a car draws, or the node whose model a floor's copy takes */
    NJS_OBJECT *object;
};

#define VARS(work) ((struct car_vars *)&(work)->vars)

/* train modes, in work->mode */
enum monorail_mode {
    MONORAIL_INIT = 0,
    MONORAIL_WAIT = 2, /* until the station calls it */
    MONORAIL_LEAVING = 3, /* on along the route from the stop */
    MONORAIL_ARRIVING = 4, /* in along the route to the stop */
    MONORAIL_STOPPED = 5, /* STOP_STEPS at the stop, then the doors open */
    MONORAIL_SHUTTING = 7, /* STOP_STEPS, then until the doors are shut */
    MONORAIL_BOARDING = 8, /* until player 0 steps into a car */
    MONORAIL_NONE = 10 /* in an act it doesn't run in */
};

enum car_mode { CAR_INIT, CAR_RUN };

/* the car player 0 has boarded */
enum monorail_car { CAR_NONE, CAR_FRONT, CAR_REAR };

static void monorail_draw(struct task *task);
static void monorail_destroy(struct task *task);
static int monorail_boarded(struct monorail *train);
static Bool monorail_car_boarded(struct task_work *work);
static struct monorail *monorail_alloc(struct task *task, NJS_MOTION *route);
static void monorail_free(struct task *task);
static void monorail_measure(struct task *task);
static void monorail_move_doors(struct monorail *train);
static Float monorail_frame_at(Float *lengths, int count, Float distance);
static void monorail_front_step(struct task *task);
static void monorail_rear_step(struct task *task);
static void monorail_car_destroy(struct task *task);
static void monorail_car_draw(struct task *task);
static void monorail_car_init(struct task *task, NJS_OBJECT *object,
                              NJS_OBJECT *solid);
static void monorail_car_place(struct task_work *work, struct monorail *train,
                               Float offset);
static void monorail_car_mark(struct task_work *work, struct monorail *train);
static void monorail_inside_create(struct task *task, NJS_OBJECT *object);
static void monorail_inside_step(struct task *task);
static void monorail_inside_destroy(struct task *task);

/* a cylinder under each car */
static struct collision_shape monorail_car_shapes[] = {
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, -10.0F, 0.0F},
     25.0F,
     10.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};

/* the pose njGetDrawMotion writes from the route, with room for six nodes.
 * the train has it pose marker_object, a single node, so only the first node's
 * values are written */
static Float monorail_pose_pos[18];
static Angle monorail_pose_ang[18];
static Float monorail_pose_scl[18];
static NJS_MOTION_DATA monorail_pose = {
    monorail_pose_pos,
    monorail_pose_ang,
    monorail_pose_scl,
};

/* the car player 0 has boarded, a monorail_car */
static int monorail_passenger = CAR_NONE;

/* each character's event flag that lets the monorail run. Eggman and Tikal
 * have none: for their -1, the step tests event_flags[-1] */
static Sint32 monorail_flags[CHARACTER_COUNT] = {
    FLAG_SONIC_EC_MONORAIL,
    -1,
    FLAG_TAILS_EC_MONORAIL,
    FLAG_KNUCKLES_EC_MONORAIL,
    -1,
    FLAG_AMY_EC_MONORAIL,
    FLAG_E102_EC_MONORAIL,
    FLAG_BIG_EC_MONORAIL,
};

/* start the doors in the mode doors, with open at from */
static void monorail_start_doors(struct monorail *train, int doors, Float from)
{
    train->doors = doors;
    train->door_steps = 0;
    train->open = from;
}

/* the train runs once the character's monorail flag is set, in acts 0 and 1
 * alone. once FLAG_MONORAIL_CALLED is set, it comes in along the route, stops
 * and opens its doors. when player 0 steps into a car, the doors shut and the
 * train leaves. LEAVE_STEPS later, it sends player 0 to the Egg Carrier
 * inside, at INSIDE_ACT and INSIDE_ENTRANCE. both players' pads are off while
 * the train comes in, and from the boarding on */
void egg_carrier_monorail_step(struct task *task)
{
    struct task_work *work = task->work;
    struct monorail *train = MONORAIL(task);
    int passenger;
    int aboard;
    Bool allowed = event_flag_check(monorail_flags[get_character()]);

    /* @hack redundant: the rest of the step in an else after the return, which
     * keeps the load of one, FLDI1 FR15 at 0x0c92ad3a, after the test of
     * allowed; with one declared above the if and no else, SHC fills the
     * test's delay slot with it, and 13 of 332 words differ */
    if (!allowed) {
        return;
    } else {
        /* @hack spelling: one, a local that holds 1.0F, which stores
         * train->open from FR15 itself (FMOV.S FR15,@(R0,R4) at 0x0c92af52);
         * with 1.0F in its three places, SHC copies it through FR4 first, and
         * 10 of 332 words differ */
        const Float one = 1.0F;

        passenger = monorail_passenger;
        switch (work->mode) {
        case MONORAIL_INIT:
            switch ((Uint8)stage_get_stage_act()) {
            case 0:
                train = monorail_alloc(task, &monorail_route_0);
                train->stop = STOP_DISTANCE;
                /* @hack redundant: a test of entrance 5 whose branches both
                 * wait, which keeps the image's call of stage_get_entrance and
                 * its CMP/EQ #5 (0x0c92adaa, 0x0c92adb0); without it,
                 * monorail.c's code runs 32 bytes short and 272 of 332 words
                 * differ */
                if (stage_get_entrance() == 5) {
                    work->mode = MONORAIL_WAIT;
                } else {
                    work->mode = MONORAIL_WAIT;
                }
                break;
            case 1:
                train = monorail_alloc(task, &monorail_route_1);
                train->stop = STOP_DISTANCE;
                /* @hack redundant: a test of entrance 2 whose branches both
                 * wait, which keeps the image's call of stage_get_entrance and
                 * its CMP/EQ #2 (0x0c92adc6, 0x0c92adcc); without it,
                 * monorail.c's code runs 32 bytes short and 263 of 332 words
                 * differ */
                if (stage_get_entrance() == 2) {
                    work->mode = MONORAIL_WAIT;
                } else {
                    work->mode = MONORAIL_WAIT;
                }
                break;
            default:
                work->mode = MONORAIL_NONE;
                return;
            }
            monorail_passenger = CAR_NONE;
            task->draw = monorail_draw;
            task->destructor = monorail_destroy;
            break;
        case MONORAIL_WAIT:
            if (event_flags[FLAG_MONORAIL_CALLED] == 1) {
                train->distance = 0.0F;
                train->speed = 5.0F;
                pad_disable_player(BOTH_PADS);
                sound_play(SE_MONORAIL_COMES, 0, 0, 0);
                work->mode = MONORAIL_ARRIVING;
            }
            return;
        case MONORAIL_ARRIVING:
            train->distance += train->speed;
            if (train->distance < train->stop - SLOW_DISTANCE) {
                train->speed = 3.0F;
            } else {
                train->speed -= train->speed * 0.05F;
                if (train->speed < one) {
                    train->speed = one;
                }
            }
            if (train->distance >= train->stop) {
                train->speed = 0.0F;
                train->distance = train->stop;
                work->timer = STOP_STEPS;
                work->mode = MONORAIL_STOPPED;
            }
            break;
        case MONORAIL_STOPPED:
            if (work->timer == 0) {
                monorail_start_doors(train, DOORS_START_OPENING, 0.0F);
                pad_enable_player(BOTH_PADS);
                sound_play(SE_MONORAIL_DOORS, 0, 0, 0);
                work->mode = MONORAIL_BOARDING;
            } else {
                work->timer--;
            }
            break;
        case MONORAIL_BOARDING:
            /* @hack redundant: 1 &&, which keeps passenger in its slot at @R15
             * (MOV.L R2,@R15 at 0x0c92ad3c); without it, passenger stays in R5,
             * the step runs 2 bytes short and 183 of 332 words differ */
            if (1 && (passenger = monorail_boarded(train))) {
                pad_disable_player(BOTH_PADS);
                monorail_start_doors(train, DOORS_START_SHUTTING, one);
                monorail_passenger = passenger;
                work->timer = STOP_STEPS;
                work->mode = MONORAIL_SHUTTING;
            }
            break;
        case MONORAIL_SHUTTING:
            if (work->timer == 0) {
                /* @hack spelling: doors, a copy of train, through which the
                 * step reads the doors' mode (MOV R14,R5 at 0x0c92afa6); read
                 * through train, the step runs 2 bytes short and 103 of 332
                 * words differ */
                struct monorail *doors = train;
                int shut = doors->doors == DOORS_SHUT;

                /* @hack redundant: a goto to the next statement, which keeps
                 * doors in R5 (0x0c92afa6, 0x0c92afa8); without it, doors takes
                 * R4 and 2 of 332 words differ */
                goto test;
            test:
                /* @hack redundant: a never-run test of shut, which keeps doors
                 * in R5 and shut in R4 (0x0c92afa6, 0x0c92afae), as the image
                 * has them; without it, the two trade and 3 of 332 words
                 * differ */
                if (0) {
                    if (shut) {
                    }
                }
                if (shut) {
                    if (passenger) {
                        player_suspend(0);
                    }
                    work->mode = MONORAIL_LEAVING;
                    train->leave_steps = 0;
                }
            } else {
                work->timer--;
            }
            break;
        case MONORAIL_LEAVING:
            train->speed -= train->speed * 0.05F;
            train->speed += 0.1F;
            train->distance += train->speed;
            /* @hack spelling: aboard, monorail_passenger read again though
             * passenger holds it, which tests the second read in R5 (MOV.L
             * @R9,R5 at 0x0c92aff2); testing passenger, from its slot at @R15,
             * 4 of 332 words differ */
            aboard = monorail_passenger;
            if (aboard) {
                if (train->leave_steps++ == LEAVE_STEPS) {
                    stage_set_entrance(INSIDE_ENTRANCE);
                    story_go_to(STAGE_EGG_CARRIER_INSIDE, INSIDE_ACT);
                }
                break;
            }
            if (train->distance >= train->length) {
                work->mode = MONORAIL_WAIT;
                event_flags[FLAG_MONORAIL_CALLED] = 0;
            }
            break;
        case MONORAIL_NONE:
            return;
        }
        monorail_move_doors(train);
        task_step_children(task);
    }
}

/* empty: each car draws itself */
static void monorail_draw(struct task *task)
{
}

static void monorail_destroy(struct task *task)
{
    task_destroy_children(task);
    monorail_free(task);
}

/* the car player 0 is in, of those the collision world tests while a player is
 * near: CAR_FRONT or CAR_REAR, or CAR_NONE */
static int monorail_boarded(struct monorail *train)
{
    if (train->front->flags & WORLD_TESTED) {
        if (monorail_car_boarded(train->front)) {
            return CAR_FRONT;
        }
    }
    if (train->rear->flags & WORLD_TESTED) {
        if (monorail_car_boarded(train->rear)) {
            return CAR_REAR;
        }
    }
    return CAR_NONE;
}

/* TRUE while player 0 is in the boarding box inside the car, tested in the
 * car's own coordinates */
static Bool monorail_car_boarded(struct task_work *work)
{
    NJS_POINT3 in;
    Bool inside = FALSE;

    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njRotateXYZ(NULL, work->ang.x, work->ang.y, work->ang.z);
    njInvertMatrix(NULL);
    njCalcPoint(NULL, &player_work[0]->pos, &in);
    if (in.x > -20.0F && in.x < 20.0F && in.y > -45.0F && in.y < -30.0F &&
        in.z > -10.0F && in.z < 10.0F) {
        inside = TRUE;
    }
    njPopMatrix(1);
    return inside;
}

/* allocate the train's record at task->motion, with its doors shut and its
 * route measured, and create its two cars. it returns the record, or null when
 * the heap runs out, and then creates no cars. egg_carrier_monorail_step
 * leaves the result unchecked */
static struct monorail *monorail_alloc(struct task *task, NJS_MOTION *route)
{
    struct monorail *train = heap_calloc(1, sizeof(struct monorail));

    if (train != NULL) {
        task->motion = (struct task_motion *)train;
        train->doors = DOORS_SHUT;
        train->route = route;
        monorail_measure(task);
        task_create_child(TASK_WORK | TASK_FORCE, monorail_front_step, task);
        task_create_child(TASK_WORK | TASK_FORCE, monorail_rear_step, task);
    }
    return train;
}

static void monorail_free(struct task *task)
{
    struct monorail *train = MONORAIL(task);

    if (task->motion != NULL) {
        if (train->lengths != NULL) {
            heap_free(train->lengths);
        }
        heap_free(task->motion);
        task->motion = NULL;
    }
}

/* measure the route from each of its frames to the next, and in all. the loop
 * stops a frame short, so the last of the lengths stays 0, and length leaves
 * it out */
static void monorail_measure(struct task *task)
{
    struct monorail *train = MONORAIL(task);
    NJS_MOTION *route = train->route;
    int count = route->nbFrame - 1;
    NJS_OBJECT *object = &marker_object;
    Float length = 0.0F;
    Float *lengths;
    NJS_POINT3 last;
    Float step;
    int i;

    lengths = heap_calloc(count, sizeof(Float));
    train->lengths = lengths;
    train->count = count;
    njGetDrawMotion(object, route, &monorail_pose, 0.0F);
    last.x = monorail_pose.pos[0];
    last.y = monorail_pose.pos[1];
    last.z = monorail_pose.pos[2];
    for (i = 1; i < count; i++) {
        njGetDrawMotion(object, route, &monorail_pose, (Float)i);
        step = njDistanceP2P(&last, (NJS_POINT3 *)monorail_pose.pos);
        length += step;
        *lengths++ = step;
        last.x = monorail_pose.pos[0];
        last.y = monorail_pose.pos[1];
        last.z = monorail_pose.pos[2];
    }
    train->length = length;
}

static void monorail_move_doors(struct monorail *train)
{
    switch (train->doors) {
    case DOORS_OPEN:
        train->open = 1.0F;
        break;
    case DOORS_SHUT:
        train->open = 0.0F;
        break;
    case DOORS_START_OPENING:
        train->open += 0.035F;
        if (train->open >= DOORS_AJAR) {
            train->open = DOORS_AJAR;
        }
        if (train->door_steps++ == DOORS_START_STEPS) {
            train->doors = DOORS_OPENING;
        }
        break;
    case DOORS_OPENING:
        train->open += 0.015F;
        if (train->open >= 1.0F) {
            train->open = 1.0F;
            train->doors = DOORS_OPEN;
        }
        break;
    case DOORS_START_SHUTTING:
        train->open -= 0.03F;
        if (train->open <= DOORS_AJAR) {
            train->open = DOORS_AJAR;
            train->doors = DOORS_SHUTTING;
        }
        break;
    case DOORS_SHUTTING:
        train->open -= 0.01F;
        if (train->open <= 0.0F) {
            train->open = 0.0F;
            train->doors = DOORS_SHUT;
        }
        break;
    }
}

/* the route frame at distance along it, from count lengths. past the route's
 * end, it returns the route's length, not a frame */
static Float monorail_frame_at(Float *lengths, int count, Float distance)
{
    Float length = 0.0F;
    Float step;
    int i;

    for (i = 0; i < count; i++) {
        step = *lengths++;
        if (length + step > distance) {
            distance = (distance - length) / step;
            if (distance < 0.0F) {
                distance = 0.0F;
            } else if (distance >= 1.0F) {
                distance = 1.0F;
            }
            return (Float)i + distance;
        }
        length += step;
    }
    return length;
}

/* task step for the front car: place it at the train's distance along the
 * route, swing its doors and draw it */
static void monorail_front_step(struct task *task)
{
    struct task_work *work = task->work;
    struct monorail *train = MONORAIL(task->parent);
    NJS_OBJECT *door;

    switch (work->mode) {
    case CAR_INIT:
        train->front = work;
        monorail_car_init(task, &monorail_front_object,
                          &monorail_front_solid_object);
        monorail_inside_create(task, &monorail_front_inside_object);
        collision_init(task, monorail_car_shapes, COLLISION_OBJECT, 1, 1);
        task->destructor = monorail_car_destroy;
        task->draw = monorail_car_draw;
        work->mode = CAR_RUN;
        return;
    case CAR_RUN:
        monorail_car_place(work, train, 0.0F);
        break;
    }
    /* the door's turns go into monorail_front_object's own nodes */
    door = &monorail_front_object;
    door = door->child;
    door->ang[0] = (Angle)(train->open * DOOR_TURN);
    door = door->child;
    door->ang[0] = (Angle)(train->open * DOOR_CHILD_TURN);
    monorail_car_mark(work, train);
    task_step_children(task);
    monorail_car_draw(task);
    collision_register(work);
}

/* task step for the rear car, REAR_OFFSET behind the front */
static void monorail_rear_step(struct task *task)
{
    struct task_work *work = task->work;
    struct monorail *train = MONORAIL(task->parent);
    NJS_OBJECT *door;

    switch (work->mode) {
    case CAR_INIT:
        train->rear = work;
        monorail_car_init(task, &monorail_rear_object,
                          &monorail_rear_solid_object);
        monorail_inside_create(task, &monorail_rear_inside_object);
        collision_init(task, monorail_car_shapes, COLLISION_OBJECT, 1, 1);
        task->destructor = monorail_car_destroy;
        task->draw = monorail_car_draw;
        work->mode = CAR_RUN;
        return;
    case CAR_RUN:
        monorail_car_place(work, train, REAR_OFFSET);
        break;
    }
    door = &monorail_rear_object;
    door = door->child;
    door->ang[0] = (Angle)(train->open * DOOR_TURN);
    door = door->child;
    door->ang[0] = (Angle)(train->open * DOOR_CHILD_TURN);
    monorail_car_mark(work, train);
    task_step_children(task);
    monorail_car_draw(task);
    collision_register(work);
}

static void monorail_car_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->solid != NULL) {
        world_remove(task, VARS(work)->solid);
        world_object_free(VARS(work)->solid);
    }
    task_destroy_children(task);
}

static void monorail_car_draw(struct task *task)
{
    struct task_work *work = task->work;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njRotateXYZ(NULL, work->ang.x, work->ang.y, work->ang.z);
    draw_tree(VARS(work)->object);
    njPopMatrix(1);
}

/* set the car to draw object, and put a copy of solid's model in the
 * collision world */
static void monorail_car_init(struct task *task, NJS_OBJECT *object,
                              NJS_OBJECT *solid)
{
    struct task_work *work = task->work;
    NJS_OBJECT *copy;

    VARS(work)->object = object;
    VARS(work)->solid = copy = world_object_alloc();
    copy->evalflags = NJD_EVAL_BREAK;
    copy->scl[0] = 1.0F;
    copy->scl[1] = 1.0F;
    copy->scl[2] = 1.0F;
    copy->ang[0] = 0;
    copy->ang[1] = 0;
    copy->ang[2] = 0;
    copy->pos[0] = 0.0F;
    copy->pos[1] = 0.0F;
    copy->pos[2] = 0.0F;
    copy->model = solid->model;
    copy->child = NULL;
    copy->sibling = NULL;
    world_add(CAR_FLAGS, task, copy);
    work->flags &= ~WORLD_TESTED;
}

/* place the car and its copy in the collision world where the route puts the
 * train's distance plus offset. while that sum is 0 or less, the car stands at
 * the route's first frame */
static void monorail_car_place(struct task_work *work, struct monorail *train,
                               Float offset)
{
    Float distance = train->distance + offset;
    Float frame;
    NJS_OBJECT *solid;

    if (distance > 0.0F) {
        frame = monorail_frame_at(train->lengths, train->count, distance);
    } else {
        frame = 0.0F;
    }
    njGetDrawMotion(&marker_object, train->route, &monorail_pose, frame);
    solid = VARS(work)->solid;
    solid->pos[0] = work->pos.x = monorail_pose.pos[0];
    solid->pos[1] = work->pos.y = monorail_pose.pos[1];
    solid->pos[2] = work->pos.z = monorail_pose.pos[2];
    solid->ang[0] = work->ang.x = monorail_pose.ang[0];
    solid->ang[1] = work->ang.y = monorail_pose.ang[1];
    solid->ang[2] = work->ang.z = monorail_pose.ang[2];
}

/* while a player is within CAR_RANGE, the collision world tests the car, and
 * CAR_OPEN says whether the doors stand fully open. out of range, CAR_OPEN
 * keeps its last value */
static void monorail_car_mark(struct task_work *work, struct monorail *train)
{
    work->flags &= ~WORLD_TESTED;
    if (player_in_range(&work->pos, CAR_RANGE) != 0) {
        work->flags |= WORLD_TESTED;
        work->flags &= ~CAR_OPEN;
        if (train->open == 1.0F) {
            work->flags |= CAR_OPEN;
        }
    }
}

/* create the car's inside floor as a child task */
static void monorail_inside_create(struct task *task, NJS_OBJECT *object)
{
    struct task *child =
        task_create_child(TASK_WORK | TASK_FORCE, monorail_inside_step, task);

    if (child != NULL) {
        VARS(child->work)->object = object;
    }
}

/* while the car is tested and open, move the inside floor to the car and let
 * the collision world test it */
static void monorail_inside_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *car = task->parent->work;
    NJS_OBJECT *solid;

    switch (work->mode) {
    case CAR_INIT:
        VARS(work)->solid = solid = world_object_alloc();
        solid->evalflags = NJD_EVAL_BREAK;
        solid->scl[0] = 1.0F;
        solid->scl[1] = 1.0F;
        solid->scl[2] = 1.0F;
        solid->ang[0] = 0;
        solid->ang[1] = work->ang.y;
        solid->ang[2] = 0;
        solid->pos[0] = work->pos.x;
        solid->pos[1] = work->pos.y;
        solid->pos[2] = work->pos.z;
        solid->model = VARS(work)->object->model;
        solid->child = NULL;
        solid->sibling = NULL;
        world_add(INSIDE_FLAGS, task, solid);
        work->flags &= ~WORLD_TESTED;
        task->destructor = monorail_inside_destroy;
        work->mode = CAR_RUN;
        break;
    case CAR_RUN:
        work->flags &= ~WORLD_TESTED;
        if ((car->flags & WORLD_TESTED) && (car->flags & CAR_OPEN)) {
            solid = VARS(work)->solid;
            solid->pos[0] = car->pos.x;
            solid->pos[1] = car->pos.y;
            solid->pos[2] = car->pos.z;
            solid->ang[0] = car->ang.x;
            solid->ang[1] = car->ang.y;
            solid->ang[2] = car->ang.z;
            work->flags |= WORLD_TESTED;
        }
        break;
    }
}

static void monorail_inside_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->solid != NULL) {
        world_remove(task, VARS(work)->solid);
        world_object_free(VARS(work)->solid);
    }
}
