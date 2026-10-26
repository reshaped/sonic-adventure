/* the train the object list names MONORAIL, inside the Egg Carrier. its two
 * cars come in when the station calls them, and carry player 0 out to the
 * deck, STAGE_EGG_CARRIER */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv0100/station.h"
#include "adv0130/bridge.h"
#include "adv0130/dat_0ca14ff0.h"
#include "adv0130/monorail.h"
#include "camera/camera.h"
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

/* each car's objects: the one it draws, then the ones whose models its solid
 * and its gangway's solid copy. a car's first child and that child's own
 * child are the two parts of its door */
extern NJS_OBJECT monorail_front_object;
extern NJS_OBJECT monorail_front_solid_object;
extern NJS_OBJECT monorail_front_gangway_object;
extern NJS_OBJECT monorail_rear_object;
extern NJS_OBJECT monorail_rear_solid_object;
extern NJS_OBJECT monorail_rear_gangway_object;
/* the route the cars follow, a motion of marker_object */
extern NJS_MOTION monorail_route_motion;

/* the front car's distance along the route at the station */
#define STOP_DISTANCE 928.0F
/* the rear car's distance along the route from the front car's */
#define REAR_OFFSET (-84.0F)

/* the train comes in at COME_SPEED until it is SLOW_DISTANCE short of its
 * stop. from there each step it loses DRAG of its speed, down to SLOW_SPEED */
#define COME_SPEED 3.0F
#define SLOW_DISTANCE 200.0F
#define DRAG 0.05F
#define SLOW_SPEED 1.0F
/* leaving, each step the train loses DRAG of its speed and gains
 * LEAVE_THRUST. its speed nears LEAVE_THRUST / DRAG, in distance along the
 * route a step */
#define LEAVE_THRUST 0.1F

/* the steps the train stands at the station before its doors open */
#define STOP_STEPS 60
/* the steps from boarding until the train waits for its doors to shut */
#define SHUT_STEPS 60
/* the steps the train runs out with a player aboard before it goes to the
 * deck */
#define LEAVE_STEPS 60

/* the entrance at which the act starts with the train shutting its doors to
 * leave, empty */
#define ENTRANCE_MONORAIL 9
/* the act and entrance of the deck at which the train lets a player out.
 * while the character's EC_TRANSFORM flag is set, they are
 * DECK_ACT_TRANSFORMED and DECK_ENTRANCE_TRANSFORMED. otherwise they are
 * DECK_ACT, act 0, and DECK_ENTRANCE */
#define DECK_ACT_TRANSFORMED 1
#define DECK_ENTRANCE_TRANSFORMED 2
#define DECK_ENTRANCE 5
/* @hack redundant: act 0 from a select whose condition is constant, which
 * egg_carrier_monorail_step loads as MOV #0,R5 at 0x0c927b0a; as a plain 0,
 * SHC copies R11's 0, and 3 of 346 words differ */
/* NOLINTNEXTLINE(clang-diagnostic-literal-conversion) */
#define DECK_ACT (2.0F ? 0 : 0)

/* the player number pad_disable_player and pad_enable_player take as both
 * players */
#define BOTH_PADS 2

#define SE_MONORAIL_COMES 563
#define SE_MONORAIL_DOORS 562

/* the flags of a car's solid in the collision world */
#define CAR_SOLID_FLAGS                                                        \
    (WORLD_MEDIUM | WORLD_MOVES | WORLD_UNCLIMBABLE | WORLD_SOLID)
#define GANGWAY_FLAGS (WORLD_MEDIUM | WORLD_SOLID)

/* in a car's work->flags, beside WORLD_TESTED: the doors stand fully open */
#define CAR_DOORS_OPEN 0x200
/* the reach within which a player has the collision world test a car */
#define CAR_NEAR 100.0F

/* how far the two parts of a car's door turn about x at fully open. the
 * first turns a sixth of a turn, and the second, on the first, three
 * sixteenths */
#define DOOR_TURN 10922.0F
#define DOOR_FOLD 12288.0F

/* the camera action's frame past which the bridge rises */
#define BRIDGE_FRAME 60.0F
/* the frames the camera task plays the action a step */
#define CAMERA_RATE 0.5F
/* the frame at which the camera task ends */
#define CAMERA_END 199.99F

enum monorail_car { CAR_NONE, CAR_FRONT, CAR_REAR };

/* the train's modes, in work->mode */
enum monorail_mode {
    MONORAIL_INIT = 0,
    MONORAIL_WAIT = 2, /* until the station calls it */
    MONORAIL_LEAVE = 3,
    MONORAIL_COME = 4,
    MONORAIL_STOP = 5, /* STOP_STEPS at the station */
    MONORAIL_SHUT = 7, /* SHUT_STEPS, then until the doors shut */
    MONORAIL_BOARD = 8, /* doors open, until player 0 boards */
    MONORAIL_IDLE = 10
};

/* the modes of a car and a gangway, and of the camera task, in work->mode */
enum part_mode { PART_INIT, PART_RUN };
enum monorail_camera_mode { CAMERA_COME, CAMERA_BRIDGE_UP };

/* the doors' modes, in doors. opening or shutting, the doors move fast as far
 * as DOORS_AJAR and slowly the rest of the way */
enum doors_mode {
    DOORS_OPEN,
    DOORS_SHUT,
    DOORS_START_OPENING, /* to DOORS_AJAR, held until START_OPEN_STEPS pass */
    DOORS_OPENING,
    DOORS_START_SHUTTING, /* to DOORS_AJAR */
    DOORS_SHUTTING
};

/* how far open the doors stand between their fast and slow parts */
#define DOORS_AJAR 0.2F
/* how far the doors move a step in each mode */
#define START_OPEN_RATE 0.035F
#define OPEN_RATE 0.015F
#define START_SHUT_RATE 0.03F
#define SHUT_RATE 0.01F
/* DOORS_START_OPENING lasts START_OPEN_STEPS + 1 steps */
#define START_OPEN_STEPS 20

/* the train's record, allocated from the heap at task->motion */
struct monorail {
    Uint8 unk_00[2];
    Sint8 doors; /* an enum doors_mode */
    Uint8 unk_03;
    Sint16 doors_steps; /* steps spent in DOORS_START_OPENING */
    Uint8 unk_06[2];
    Sint32 steps_out; /* steps since the train left with a player aboard */
    Float distance; /* how far along the route the front car is */
    Float speed;
    Uint8 unk_14[4];
    Float open; /* how far open the doors are, from 0 to 1 */
    Float stop; /* the front car's distance along the route at its stop */
    Uint8 unk_20[4];
    NJS_MOTION *route;
    Float *segments; /* the distance from each frame of the route to the next */
    Float length; /* the segments' sum */
    Sint32 segment_count;
    /* the two cars' works, which each car's first step sets */
    struct task_work *front;
    struct task_work *rear;
};

#define MONORAIL(task) ((struct monorail *)(task)->motion)

/* the vars of a car, a gangway or the camera task */
struct monorail_vars {
    NJS_OBJECT *solid; /* in the collision world */
    NJS_OBJECT *object; /* what a car draws, or a gangway's solid copies */
    Float frame; /* the camera task's action frame */
};

#define VARS(work) ((struct monorail_vars *)&(work)->vars)

/* each character's EC_TRANSFORM flag, which picks the act a player who rides
 * the train out goes to. Eggman and Tikal have -1 here and in
 * monorail_run_flags, so event_flag_check reads the byte before event_flags
 * for them */
static int monorail_transform_flags[CHARACTER_COUNT] = {
    FLAG_SONIC_EC_TRANSFORM,
    -1,
    FLAG_TAILS_EC_TRANSFORM,
    FLAG_KNUCKLES_EC_TRANSFORM,
    -1,
    FLAG_AMY_EC_TRANSFORM,
    FLAG_E102_EC_TRANSFORM,
    FLAG_BIG_EC_TRANSFORM,
};

static struct collision_shape monorail_car_shape[1] = {
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, -10.0F, 0.0F},
     25.0F,
     10.0F,
     0.0F,
     {0},
     0,
     NULL},
};

/* the nodes monorail_pose has room for */
#define POSE_NODES 6

/* the pose the route gives a car, which njGetDrawMotion fills: a position,
 * angles and a scale for each node */
static Float monorail_pose_pos[POSE_NODES * 3];
static Angle monorail_pose_ang[POSE_NODES * 3];
static Float monorail_pose_scl[POSE_NODES * 3];
static NJS_MOTION_DATA monorail_pose = {monorail_pose_pos, monorail_pose_ang,
                                        monorail_pose_scl};

/* the car player 0 has boarded, an enum monorail_car */
static int monorail_aboard = CAR_NONE;

/* each character's EC_MONORAIL flag. until it is set the train does nothing */
static int monorail_run_flags[CHARACTER_COUNT] = {
    FLAG_SONIC_EC_MONORAIL,
    -1,
    FLAG_TAILS_EC_MONORAIL,
    FLAG_KNUCKLES_EC_MONORAIL,
    -1,
    FLAG_AMY_EC_MONORAIL,
    FLAG_E102_EC_MONORAIL,
    FLAG_BIG_EC_MONORAIL,
};

static void monorail_draw(struct task *task);
static void monorail_destroy(struct task *task);
static void monorail_camera_step(struct task *task);
static int monorail_aboard_car(struct monorail *monorail);
static Bool monorail_car_holds(struct task_work *work);
static struct monorail *monorail_init(struct task *task, NJS_MOTION *route);
static void monorail_free(struct task *task);
static void monorail_measure(struct task *task);
static void monorail_move_doors(struct monorail *monorail);
static Float monorail_frame_at(Float *segments, int count, Float distance);
static void monorail_front_step(struct task *task);
static void monorail_rear_step(struct task *task);
static void monorail_car_destroy(struct task *task);
static void monorail_car_draw(struct task *task);
static void monorail_car_init(struct task *task, NJS_OBJECT *object,
                              NJS_OBJECT *solid);
static void monorail_car_move(struct task_work *work, struct monorail *monorail,
                              Float offset);
static void monorail_car_mark(struct task_work *work,
                              struct monorail *monorail);
static void monorail_gangway_create(struct task *task, NJS_OBJECT *object);
static void monorail_gangway_step(struct task *task);
static void monorail_gangway_destroy(struct task *task);

static void monorail_open_doors(struct monorail *monorail)
{
    monorail->doors = DOORS_START_OPENING;
    monorail->doors_steps = 0;
    monorail->open = 0.0F;
}

static void monorail_shut_doors(struct monorail *monorail)
{
    monorail->doors = DOORS_START_SHUTTING;
    monorail->doors_steps = 0;
    monorail->open = 1.0F;
}

void egg_carrier_monorail_step(struct task *task)
{
    struct task_work *work = task->work;
    struct monorail *monorail = MONORAIL(task);
    int aboard;
    int car;
    int act;
    Bool transformed;
    Bool runs = event_flag_check(monorail_run_flags[get_character()]);

    /* @hack redundant: the step's rest in an else after the return, which loads
     * monorail_aboard into R2 (0x0c92783a); after the if, into R3, and 2 of
     * 346 words differ */
    if (!runs) {
        return;
    } else {
        aboard = monorail_aboard;
        /* @hack redundant: a never-run test of aboard, which keeps aboard at
         * @R15 (0x0c927840); without it, aboard stays in R5, the function
         * runs 2 bytes shorter and 224 of 346 words differ */
        if (0) {
            if (aboard && (aboard = 0)) {
            }
        }
        switch (work->mode) {
        case MONORAIL_INIT:
            monorail = monorail_init(task, &monorail_route_motion);
            monorail->stop = STOP_DISTANCE;
            if (stage_get_entrance() == ENTRANCE_MONORAIL) {
                pad_disable_player(BOTH_PADS);
                monorail_shut_doors(monorail);
                work->mode = MONORAIL_SHUT;
            } else {
                work->mode = MONORAIL_WAIT;
            }
            monorail_aboard = CAR_NONE;
            task->draw = monorail_draw;
            task->destructor = monorail_destroy;
            break;
        case MONORAIL_WAIT:
            if (event_flags[FLAG_MONORAIL_CALLED] == 1) {
                monorail->distance = 0.0F;
                monorail->speed = COME_SPEED;
                pad_disable_player(BOTH_PADS);
                sound_play(SE_MONORAIL_COMES, 0, 0, 0);
                task_create_child(TASK_WORK, monorail_camera_step, task);
                work->mode = MONORAIL_COME;
            }
            return;
        case MONORAIL_COME:
            monorail->distance += monorail->speed;
            if (monorail->distance < monorail->stop - SLOW_DISTANCE) {
                monorail->speed = COME_SPEED;
            } else {
                monorail->speed -= monorail->speed * DRAG;
                if (monorail->speed < SLOW_SPEED) {
                    monorail->speed = SLOW_SPEED;
                }
            }
            if (monorail->distance >= monorail->stop) {
                monorail->speed = 0.0F;
                monorail->distance = monorail->stop;
                work->timer = STOP_STEPS;
                work->mode = MONORAIL_STOP;
            }
            break;
        case MONORAIL_STOP:
            if (work->timer == 0) {
                monorail_open_doors(monorail);
                pad_enable_player(BOTH_PADS);
                sound_play(SE_MONORAIL_DOORS, 0, 0, 0);
                work->mode = MONORAIL_BOARD;
            } else {
                work->timer--;
            }
            break;
        case MONORAIL_BOARD: {
            /* @hack spelling: copy, which holds monorail, so the doors are
             * read through R5 (MOV R14,R5 at 0x0c927a12); through monorail, 31
             * of 346 words differ */
            struct monorail *copy = monorail;
            Bool wide = copy->doors == DOORS_OPEN;

            /* @hack redundant: a never-run test of copy, which keeps it in R5
             * (0x0c927a12), as the image has it; without it, copy takes R4 and
             * 2 of 346 words differ */
            if (0) {
                if (copy) {
                }
            }
            /* @hack redundant: a never-run test of wide + wide, which keeps
             * copy in R5 and wide in R4 (0x0c927a12, 0x0c927a1a), as the image
             * has them; without it, the two trade and 3 of 346 words differ */
            if (0) {
                if (wide + wide) {
                }
            }
            if (!wide) {
                break;
            }
            if ((aboard = monorail_aboard_car(monorail))) {
                pad_disable_player(BOTH_PADS);
                monorail_shut_doors(monorail);
                monorail_aboard = aboard;
                work->timer = SHUT_STEPS;
                work->mode = MONORAIL_SHUT;
            }
        } break;
        case MONORAIL_SHUT:
            if (work->timer == 0) {
                /* @hack spelling: copy, which holds monorail and which the
                 * case copies back, so the doors are read through R4 (MOV
                 * R14,R4 at 0x0c927a86); through monorail, the function runs 2
                 * bytes shorter and 122 of 346 words differ */
                struct monorail *copy = monorail;
                Bool shut = copy->doors == DOORS_SHUT;

                /* @hack redundant: the copy back, which keeps shut in R5
                 * (MOVT R5 at 0x0c927a8e); without it, shut takes R4 and 1 of
                 * 346 words differ */
                monorail = copy;
                if (!shut) {
                    break;
                }
                if (aboard) {
                    player_suspend(0);
                }
                work->mode = MONORAIL_LEAVE;
                monorail->steps_out = 0;
            } else {
                work->timer--;
            }
            break;
        case MONORAIL_LEAVE:
            monorail->speed -= monorail->speed * DRAG;
            monorail->speed += LEAVE_THRUST;
            monorail->distance += monorail->speed;
            car = monorail_aboard;
            if (car) {
                if (monorail->steps_out++ == LEAVE_STEPS) {
                    /* each branch below sets the entrance again, over this
                     * first set */
                    stage_set_entrance(DECK_ENTRANCE_TRANSFORMED);
                    transformed = event_flag_check(
                        monorail_transform_flags[get_character()]);
                    if (transformed) {
                        stage_set_entrance(DECK_ENTRANCE_TRANSFORMED);
                        act = DECK_ACT_TRANSFORMED;
                    } else {
                        stage_set_entrance(DECK_ENTRANCE);
                        /* @hack redundant: an empty never-run block, which,
                         * with DECK_ACT's constant select, keeps
                         * story_go_to's address in R2 (0x0c927b0c), as the
                         * image has it; without it, R3 takes it and 2 of 346
                         * words differ */
                        if (0) {
                        }
                        act = DECK_ACT;
                    }
                    story_go_to(STAGE_EGG_CARRIER, act);
                }
                break;
            }
            if (monorail->distance >= monorail->length) {
                work->mode = MONORAIL_WAIT;
                event_flags[FLAG_MONORAIL_CALLED] = 0;
            }
            break;
        case MONORAIL_IDLE:
            return;
        }
        monorail_move_doors(monorail);
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

/* play monorail_camera_action as the train comes in. past BRIDGE_FRAME it
 * sets FLAG_BRIDGE_UP, and at CAMERA_END the task kills itself */
static void monorail_camera_step(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->frame += CAMERA_RATE;
    switch (work->mode) {
    case CAMERA_COME:
        if (VARS(work)->frame > BRIDGE_FRAME) {
            event_flags[FLAG_BRIDGE_UP] = 1;
            work->mode = CAMERA_BRIDGE_UP;
        }
        break;
    case CAMERA_BRIDGE_UP:
        if (VARS(work)->frame >= CAMERA_END) {
            task_kill(task);
            return;
        }
        break;
    }
    camera_play_action(&monorail_camera_action, VARS(work)->frame);
}

/* the car whose boarding box holds player 0, of the cars a player is near,
 * or CAR_NONE */
static int monorail_aboard_car(struct monorail *monorail)
{
    if (monorail->front->flags & WORLD_TESTED) {
        if (monorail_car_holds(monorail->front)) {
            return CAR_FRONT;
        }
    }
    if (monorail->rear->flags & WORLD_TESTED) {
        if (monorail_car_holds(monorail->rear)) {
            return CAR_REAR;
        }
    }
    return CAR_NONE;
}

/* TRUE while player 0 is in the boarding box inside the car, a box in the
 * car's own coordinates. otherwise FALSE */
static Bool monorail_car_holds(struct task_work *work)
{
    NJS_POINT3 pos;
    Bool inside = FALSE;

    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njRotateXYZ(NULL, work->ang.x, work->ang.y, work->ang.z);
    njInvertMatrix(NULL);
    njCalcPoint(NULL, &player_work[0]->pos, &pos);
    if (pos.x > -20.0F && pos.x < 20.0F && pos.y > -45.0F && pos.y < -30.0F &&
        pos.z > -10.0F && pos.z < 10.0F) {
        inside = TRUE;
    }
    njPopMatrix(1);
    return inside;
}

/* allocate the train's record at task->motion, measure route, and make the
 * two cars as children. return the record, or null when the heap runs out.
 * egg_carrier_monorail_step leaves the result unchecked */
static struct monorail *monorail_init(struct task *task, NJS_MOTION *route)
{
    struct monorail *monorail = heap_calloc(1, sizeof(struct monorail));

    if (monorail != NULL) {
        task->motion = (struct task_motion *)monorail;
        monorail->doors = DOORS_SHUT;
        monorail->route = route;
        monorail_measure(task);
        task_create_child(TASK_WORK | TASK_FORCE, monorail_front_step, task);
        task_create_child(TASK_WORK | TASK_FORCE, monorail_rear_step, task);
    }
    return monorail;
}

static void monorail_free(struct task *task)
{
    struct monorail *monorail = MONORAIL(task);

    if (task->motion != NULL) {
        if (monorail->segments != NULL) {
            heap_free(monorail->segments);
        }
        heap_free(task->motion);
        task->motion = NULL;
    }
}

/* fill segments with the distance from each of the route's frames to the
 * next. the route has nbFrame - 1 segments, but the loop measures one fewer,
 * so the last stays 0 and length leaves it out */
static void monorail_measure(struct task *task)
{
    struct monorail *monorail = MONORAIL(task);
    NJS_MOTION *motion = monorail->route;
    int count = motion->nbFrame - 1;
    NJS_OBJECT *object = &marker_object;
    Float total = 0.0F;
    Float *segments;
    NJS_POINT3 last;
    Float length;
    int i;

    segments = heap_calloc(count, sizeof(Float));
    monorail->segments = segments;
    monorail->segment_count = count;
    njGetDrawMotion(object, motion, &monorail_pose, 0.0F);
    last.x = monorail_pose.pos[0];
    last.y = monorail_pose.pos[1];
    last.z = monorail_pose.pos[2];
    for (i = 1; i < count; i++) {
        njGetDrawMotion(object, motion, &monorail_pose, (Float)i);
        length = njDistanceP2P(&last, (NJS_POINT3 *)monorail_pose.pos);
        total += length;
        *segments++ = length;
        last.x = monorail_pose.pos[0];
        last.y = monorail_pose.pos[1];
        last.z = monorail_pose.pos[2];
    }
    monorail->length = total;
}

static void monorail_move_doors(struct monorail *monorail)
{
    switch (monorail->doors) {
    case DOORS_OPEN:
        monorail->open = 1.0F;
        break;
    case DOORS_SHUT:
        monorail->open = 0.0F;
        break;
    case DOORS_START_OPENING:
        monorail->open += START_OPEN_RATE;
        if (monorail->open >= DOORS_AJAR) {
            monorail->open = DOORS_AJAR;
        }
        if (monorail->doors_steps++ == START_OPEN_STEPS) {
            monorail->doors = DOORS_OPENING;
        }
        break;
    case DOORS_OPENING:
        monorail->open += OPEN_RATE;
        if (monorail->open >= 1.0F) {
            monorail->open = 1.0F;
            monorail->doors = DOORS_OPEN;
        }
        break;
    case DOORS_START_SHUTTING:
        monorail->open -= START_SHUT_RATE;
        if (monorail->open <= DOORS_AJAR) {
            monorail->open = DOORS_AJAR;
            monorail->doors = DOORS_SHUTTING;
        }
        break;
    case DOORS_SHUTTING:
        monorail->open -= SHUT_RATE;
        if (monorail->open <= 0.0F) {
            monorail->open = 0.0F;
            monorail->doors = DOORS_SHUT;
        }
        break;
    }
}

/* the route's frame at distance along it, from count segments. past the
 * segments' end it returns their sum, a distance, not a frame */
static Float monorail_frame_at(Float *segments, int count, Float distance)
{
    Float total = 0.0F;
    Float length;
    int i;

    for (i = 0; i < count; i++) {
        length = *segments++;
        if (total + length > distance) {
            distance = (distance - total) / length;
            if (distance < 0.0F) {
                distance = 0.0F;
            } else if (distance >= 1.0F) {
                distance = 1.0F;
            }
            return (Float)i + distance;
        }
        total += length;
    }
    return total;
}

static void monorail_front_step(struct task *task)
{
    struct task_work *work = task->work;
    struct monorail *monorail = MONORAIL(task->parent);
    NJS_OBJECT *door;

    switch (work->mode) {
    case PART_INIT:
        monorail->front = work;
        monorail_car_init(task, &monorail_front_object,
                          &monorail_front_solid_object);
        monorail_gangway_create(task, &monorail_front_gangway_object);
        collision_init(task, monorail_car_shape, COLLISION_OBJECT, 1, 1);
        task->destructor = monorail_car_destroy;
        task->draw = monorail_car_draw;
        work->mode = PART_RUN;
        return;
    case PART_RUN:
        monorail_car_move(work, monorail, 0.0F);
        break;
    }
    door = &monorail_front_object;
    door = door->child;
    door->ang[0] = (Angle)(monorail->open * DOOR_TURN);
    door = door->child;
    door->ang[0] = (Angle)(monorail->open * DOOR_FOLD);
    monorail_car_mark(work, monorail);
    task_step_children(task);
    monorail_car_draw(task);
    collision_register(work);
}

static void monorail_rear_step(struct task *task)
{
    struct task_work *work = task->work;
    struct monorail *monorail = MONORAIL(task->parent);
    NJS_OBJECT *door;

    switch (work->mode) {
    case PART_INIT:
        monorail->rear = work;
        monorail_car_init(task, &monorail_rear_object,
                          &monorail_rear_solid_object);
        monorail_gangway_create(task, &monorail_rear_gangway_object);
        collision_init(task, monorail_car_shape, COLLISION_OBJECT, 1, 1);
        task->destructor = monorail_car_destroy;
        task->draw = monorail_car_draw;
        work->mode = PART_RUN;
        return;
    case PART_RUN:
        monorail_car_move(work, monorail, REAR_OFFSET);
        break;
    }
    door = &monorail_rear_object;
    door = door->child;
    door->ang[0] = (Angle)(monorail->open * DOOR_TURN);
    door = door->child;
    door->ang[0] = (Angle)(monorail->open * DOOR_FOLD);
    monorail_car_mark(work, monorail);
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

/* have the car draw object, and put a world object with solid's model in
 * the collision world */
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
    world_add(CAR_SOLID_FLAGS, task, copy);
    work->flags &= ~WORLD_TESTED;
}

/* move the car and its solid to the route's pose at offset along the route
 * from the front car. a car at or short of the route's start takes frame 0 */
static void monorail_car_move(struct task_work *work, struct monorail *monorail,
                              Float offset)
{
    Float distance = monorail->distance + offset;
    Float frame;
    NJS_OBJECT *solid;

    if (distance > 0.0F) {
        frame = monorail_frame_at(monorail->segments, monorail->segment_count,
                                  distance);
    } else {
        frame = 0.0F;
    }
    njGetDrawMotion(&marker_object, monorail->route, &monorail_pose, frame);
    solid = VARS(work)->solid;
    solid->pos[0] = work->pos.x = monorail_pose.pos[0];
    solid->pos[1] = work->pos.y = monorail_pose.pos[1];
    solid->pos[2] = work->pos.z = monorail_pose.pos[2];
    solid->ang[0] = work->ang.x = monorail_pose.ang[0];
    solid->ang[1] = work->ang.y = monorail_pose.ang[1];
    solid->ang[2] = work->ang.z = monorail_pose.ang[2];
}

/* have the collision world test the car while a player is near it, and mark
 * CAR_DOORS_OPEN while the doors stand fully open */
static void monorail_car_mark(struct task_work *work, struct monorail *monorail)
{
    work->flags &= ~WORLD_TESTED;
    if (player_in_range(&work->pos, CAR_NEAR) != 0) {
        work->flags |= WORLD_TESTED;
        work->flags &= ~CAR_DOORS_OPEN;
        if (monorail->open == 1.0F) {
            work->flags |= CAR_DOORS_OPEN;
        }
    }
}

static void monorail_gangway_create(struct task *task, NJS_OBJECT *object)
{
    struct task *gangway =
        task_create_child(TASK_WORK | TASK_FORCE, monorail_gangway_step, task);

    if (gangway != NULL) {
        VARS(gangway->work)->object = object;
    }
}

/* the gangway is solid, at its car, only while the car is tested and its
 * doors stand fully open */
static void monorail_gangway_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *car = task->parent->work;
    NJS_OBJECT *solid;

    switch (work->mode) {
    case PART_INIT:
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
        world_add(GANGWAY_FLAGS, task, solid);
        work->flags &= ~WORLD_TESTED;
        task->destructor = monorail_gangway_destroy;
        work->mode = PART_RUN;
        break;
    case PART_RUN:
        work->flags &= ~WORLD_TESTED;
        if ((car->flags & WORLD_TESTED) && (car->flags & CAR_DOORS_OPEN)) {
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

static void monorail_gangway_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->solid != NULL) {
        world_remove(task, VARS(work)->solid);
        world_object_free(VARS(work)->solid);
    }
}
