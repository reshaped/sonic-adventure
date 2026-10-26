/* O TENBOU ELE, O HOTEL ELE and O HIGH ELE elevators to the lookout, the Chao
 * Garden and the highway. O ROUKA and the lights on the way to Twinkle Park are
 * here too */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/elevators.h"
#include "adv00/station_square.h"
#include "adv00/texlists.h"
#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "progress/progress.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT elevator_object;
extern NJS_OBJECT highway_elevator_object;
extern NJS_OBJECT twinkle_light_object;
extern NJS_OBJECT twinkle_light_lit_object;

/* player 0 opens the doors from within DOOR_REACH across and DOOR_HEIGHT above
 * or below the elevator */
#define DOOR_REACH 20.0F
#define DOOR_HEIGHT 20.0F
#define DOOR_OPEN 5.0F /* each door's slide when open */
#define DOOR_SPEED 0.1F /* the doors' slide a step */
/* half a door's width. shut, each door's center lies this far out from the
 * middle */
#define DOOR_HALF 5.0F
#define SE_DOORS 531 /* the doors starting to open or shut */

#define DOOR_WAIT 110 /* the doors stay open more than this many steps */
/* past DOOR_WAIT, the doors shut once player 0 is no longer within CLOSE_REACH
 * of the elevator, across and above or below */
#define CLOSE_REACH 30.0F
/* player 0 is inside while within INSIDE_REACH across and above or below a
 * point INSIDE_DEPTH behind the doors */
#define INSIDE_DEPTH 18.0F
#define INSIDE_REACH 10.0F
/* the doors shut once player 0 has stood inside more than this many steps in a
 * row */
#define RIDE_WAIT 50

#define LOOKOUT_STAGE STAGE_CHAOS2
#define HOTEL_STAGE STAGE_ALIFE_SS
#define HIGHWAY_STAGE STAGE_HIGHWAY

#define FLOOR_WORLD_FLAGS (WORLD_SMALL | WORLD_SOLID)

/* the elevator's work->mode, which skips 4 and 6: no code sets them */
enum elevator_mode {
    DOORS_SHUT = 0,
    DOORS_OPENING = 1,
    DOORS_OPEN = 2,
    DOORS_CLOSING = 3,
    DOORS_LEAVING = 5, /* shutting on player 0, to leave */
    DOORS_GONE = 7 /* shut, with the fade asked for */
};

/* the first LIGHTS_ALONG_X lights stand in a row along x, and the rest, turned
 * a quarter turn, in a row along z */
#define LIGHTS 29
#define LIGHTS_ALONG_X 18
/* the unlit model shows while twinkle_lights_vars.steps runs from 1 to
 * LIGHT_STEPS - 1. the lit one shows from LIGHT_STEPS to 2 * LIGHT_STEPS,
 * where the count starts over */
#define LIGHT_STEPS 70
#define LIGHT_Y 50.01F /* every light's height */
#define LIGHT_SIDE 6.0F /* each light's offset to one side of its place */
#define LIT_TEXTURE 12 /* the lit model's texture */

struct elevator_vars {
    /* the elevator's model, or for the floor task, its object in the world */
    NJS_OBJECT *object;
    Bool (*open_test)(void); /* the test elevator_wait_open waits to pass */
    int inside; /* steps in a row player 0 has stood inside */
};

struct twinkle_lights_vars {
    Uint8 unk_00[4];
    int steps; /* steps into the two models' cycle */
};

#define VARS(work) ((struct elevator_vars *)&(work)->vars)
#define LIGHTS_VARS(work) ((struct twinkle_lights_vars *)&(work)->vars)
#define DOORS(work) ((work)->scl.x) /* each door's slide from shut */
#define TO_STAGE(work) ((work)->tick) /* the stage the elevator leads to */
#define TO_ACT(work) ((work)->submode) /* the act it leads to */
#define LIT(work) ((work)->submode) /* 1 while the lit model shows */
/* the stage and act the lights started in, as stage_get_stage_act gives
 * them */
#define LIGHTS_STAGE_ACT(work) TASK_TIMER(work)

static Bool highway_elevator_open(void);
static Bool lookout_elevator_open(void);
static void elevator_step(struct task *task);
static void elevator_wait_open(struct task *task);
static void elevator_draw(struct task *task);
static void elevator_destroy(struct task *task);
static void elevator_place_shapes(struct task_work *work);
static void elevator_floor_init(struct task *task);
static void elevator_floor_step(struct task *task);
static void elevator_floor_destroy(struct task *task);
static void twinkle_lights_init(struct task *task);
static void twinkle_lights_step(struct task *task);
static void twinkle_lights_draw(struct task *task);
static void twinkle_lights_destroy(struct task *task);

/* shapes of the lookout and Chao Garden elevators: the two sliding doors, a
 * wall across the shut doorway, and its two jambs */
/* clang-format off */
static struct collision_shape elevator_shapes[] = {
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {5.0F, 12.5F, 0.0F},
     5.0F, 12.5F, 1.0F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {-5.0F, 12.5F, 0.0F},
     5.0F, 12.5F, 1.0F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, 12.5F, -0.5F},
     12.0F, 12.5F, 1.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {12.0F, 12.5F, 0.0F},
     3.0F, 12.5F, 1.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {-12.0F, 12.5F, 0.0F},
     3.0F, 12.5F, 1.0F, {0}, 0, NULL},
};
/* shapes of the highway elevator, taller and narrower */
static struct collision_shape highway_elevator_shapes[] = {
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {5.0F, 12.5F, 0.0F},
     5.0F, 20.0F, 1.0F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {-5.0F, 12.5F, 0.0F},
     5.0F, 20.0F, 1.0F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, 12.5F, -0.5F},
     10.0F, 20.0F, 1.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {10.0F, 12.5F, 0.0F},
     3.0F, 20.0F, 1.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {-10.0F, 12.5F, 0.0F},
     3.0F, 20.0F, 1.0F, {0}, 0, NULL},
};
/* clang-format on */

/* the unlit model's textures, of which only the first is used */
static Uint32 twinkle_light_textures[] = {10, 11, 14};
/* each light's x and z */
static Float twinkle_light_places[LIGHTS][2] = {
#include "adv00/elevators/twinkle_light_places.inc"
};

/* TRUE for Sonic or Tails once his story opens the way to the highway */
static Bool highway_elevator_open(void)
{
    switch (player_get_character(0)) {
    case CHARACTER_SONIC:
        return event_flags[FLAG_SONIC_SS_ENTRANCE_HIGHWAY];
    case CHARACTER_TAILS:
        return event_flags[FLAG_TAILS_SS_ENTRANCE_HIGHWAY];
    case CHARACTER_KNUCKLES:
    case CHARACTER_AMY:
    case CHARACTER_E102:
    case CHARACTER_BIG:
        break;
    }
    return FALSE;
}

/* TRUE for Knuckles while FLAG_KNUCKLES_LOOKOUT is set and
 * FLAG_KNUCKLES_CLEAR_CHAOS2 isn't. his story sets the first as it plays
 * event 160 */
static Bool lookout_elevator_open(void)
{
    switch (player_get_character(0)) {
    case CHARACTER_SONIC:
        /* @hack redundant: a break of Sonic's own, which leaves R2 to
         * event_flag_check's address (0x0c90b5a0, 0x0c90b5ac). with Sonic's
         * case joined to Tails', it takes R3 and 4 of 40 words differ */
        break;
    case CHARACTER_TAILS:
        break;
    case CHARACTER_KNUCKLES:
        return event_flag_check(FLAG_KNUCKLES_LOOKOUT) &&
               !event_flag_check(FLAG_KNUCKLES_CLEAR_CHAOS2);
    case CHARACTER_AMY:
    case CHARACTER_E102:
    case CHARACTER_BIG:
        break;
    }
    return FALSE;
}

void lookout_elevator_init(struct task *task)
{
    struct task_work *work = task->work;

    work->ang.x = 0;
    work->ang.z = 0;
    DOORS(work) = 0.0F;
    work->mode = DOORS_SHUT;
    TO_STAGE(work) = LOOKOUT_STAGE;
    TO_ACT(work) = 0;
    VARS(work)->object = &elevator_object;
    if (!placement_editing(task)) {
        collision_init(task, elevator_shapes, COLLISION_OBJECT, 1, 5);
        task_create_child(TASK_WORK | TASK_FORCE, elevator_floor_init, task);
    }
    if (lookout_elevator_open()) {
        task->step = elevator_step;
    } else {
        VARS(work)->open_test = lookout_elevator_open;
        task->step = elevator_wait_open;
    }
    task->draw = elevator_draw;
    task->destructor = elevator_destroy;
}

/* the Chao Garden elevator is always open */
void hotel_elevator_init(struct task *task)
{
    struct task_work *work = task->work;

    work->ang.x = 0;
    work->ang.z = 0;
    DOORS(work) = 0.0F;
    work->mode = DOORS_SHUT;
    TO_STAGE(work) = HOTEL_STAGE;
    TO_ACT(work) = 0;
    VARS(work)->object = &elevator_object;
    if (!placement_editing(task)) {
        collision_init(task, elevator_shapes, COLLISION_OBJECT, 1, 5);
        task_create_child(TASK_WORK | TASK_FORCE, elevator_floor_init, task);
    }
    task->step = elevator_step;
    task->draw = elevator_draw;
    task->destructor = elevator_destroy;
}

void highway_elevator_init(struct task *task)
{
    struct task_work *work = task->work;

    work->ang.x = 0;
    work->ang.z = 0;
    DOORS(work) = 0.0F;
    work->mode = DOORS_SHUT;
    TO_STAGE(work) = HIGHWAY_STAGE;
    TO_ACT(work) = 0;
    VARS(work)->object = &highway_elevator_object;
    if (!placement_editing(task)) {
        collision_init(task, highway_elevator_shapes, COLLISION_OBJECT, 1, 5);
        task_create_child(TASK_WORK | TASK_FORCE, elevator_floor_init, task);
    }
    if (highway_elevator_open()) {
        task->step = elevator_step;
    } else {
        VARS(work)->open_test = highway_elevator_open;
        task->step = elevator_wait_open;
    }
    task->draw = elevator_draw;
    task->destructor = elevator_destroy;
}

/* open the doors when player 0 comes near, and once DOOR_WAIT is up, shut them
 * when player 0 moves away. once player 0 has stood inside more than RIDE_WAIT
 * steps in a row, hold player 0 still and shut the doors. then fade to the
 * elevator's stage */
static void elevator_step(struct task *task)
{
    struct task_work *work = task->work;
    int act;
    int stage;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case DOORS_SHUT:
        if (player_in_cylinder(&work->pos, DOOR_REACH, DOOR_HEIGHT)) {
            work->mode = DOORS_OPENING;
            sound_play(SE_DOORS, 0, 0, 0);
        }
        break;
    case DOORS_OPENING: {
        Float doors = DOORS(work);

        doors += DOOR_SPEED;
        if (doors > DOOR_OPEN) {
            doors = DOOR_OPEN;
            work->timer = 0;
            VARS(work)->inside = 0;
            work->mode = DOORS_OPEN;
        }
        DOORS(work) = doors;
        if (!placement_editing(task)) {
            elevator_place_shapes(work);
        }
        break;
    }
    case DOORS_OPEN: {
        Float z;
        Float x;

        if (TASK_TIMER(work) > DOOR_WAIT) {
            if (!player_in_cylinder(&work->pos, CLOSE_REACH, CLOSE_REACH)) {
                work->timer = 0;
                work->mode = DOORS_CLOSING;
                sound_play(SE_DOORS, 0, 0, 0);
            }
        } else {
            TASK_TIMER(work)++;
        }
        /* move pos to the point inside for the test, and back after it */
        x = work->pos.x;
        z = work->pos.z;
        work->pos.x += -(njSin(work->ang.y) * INSIDE_DEPTH);
        work->pos.z += -(njCos(work->ang.y) * INSIDE_DEPTH);
        if (player_in_cylinder(&work->pos, INSIDE_REACH, INSIDE_REACH)) {
            VARS(work)->inside++;
            if (VARS(work)->inside > RIDE_WAIT) {
                work->mode = DOORS_LEAVING;
                sound_play(SE_DOORS, 0, 0, 0);
                player_set_mode(0, PLAYER_MODE_STILL);
                player_set_heading(0, work->ang.y);
            }
        } else {
            VARS(work)->inside = 0;
        }
        work->pos.x = x;
        work->pos.z = z;
        break;
    }
    case DOORS_CLOSING: {
        Float doors = DOORS(work);

        doors -= DOOR_SPEED;
        if (doors < 0.0F) {
            doors = 0.0F;
            work->timer = 0;
            work->mode = DOORS_SHUT;
        }
        DOORS(work) = doors;
        if (!placement_editing(task)) {
            elevator_place_shapes(work);
        }
        break;
    }
    case DOORS_LEAVING: {
        Float doors = DOORS(work);

        doors -= DOOR_SPEED;
        if (doors < 0.0F) {
            doors = 0.0F;
            work->timer = 0;
            player_set_mode(0, PLAYER_MODE_RESET);
            stage_set_entrance(0);
            stage = TO_STAGE(work);
            /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
            act = TO_ACT(work);
            stage_fade_to(stage, act);
            work->mode = DOORS_GONE;
        }
        DOORS(work) = doors;
        break;
    }
    case DOORS_GONE:
        break;
    }
    if (!placement_editing(task)) {
        collision_register(work);
        task_step_children(task);
    }
    elevator_draw(task);
}

/* keep the elevator shut until its open test passes */
static void elevator_wait_open(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (!placement_editing(task)) {
        if (VARS(work)->open_test) {
            Bool (*open_test)(void) = VARS(work)->open_test;

            if (open_test()) {
                work->mode = DOORS_SHUT;
                task->step = elevator_step;
            }
        }
        collision_register(work);
        task_step_children(task);
    }
    elevator_draw(task);
}

/* draw the elevator's body, then each door and its inner panel. the second door
 * is turned a half turn, and each inner panel slides twice as far as its
 * door */
static void elevator_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;
    NJS_OBJECT *inner;
    Float slide;
    Angle ang;

    texture_set_stage();
    slide = -DOORS(work);
    object = VARS(work)->object;
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(object->model);
    object = object->child;
    njPushMatrix(NULL);
    njTranslate(NULL, slide, 0.0F, 0.0F);
    draw_model(object->model);
    inner = object->child;
    njTranslate(NULL, slide, 0.0F, 0.0F);
    draw_model(inner->model);
    njPopMatrix(1);
    object = object->sibling;
    njPushMatrix(NULL);
    njRotateY(NULL, 0x8000);
    njTranslate(NULL, slide, 0.0F, 0.0F);
    draw_model(object->model);
    object = object->child;
    njTranslate(NULL, slide, 0.0F, 0.0F);
    draw_model(object->model);
    njPopMatrix(1);
    njPopMatrix(1);
}

static void elevator_destroy(struct task *task)
{
    task_destroy_children(task);
}

/* while shut, make the doorway's wall and the jambs solid. otherwise make the
 * jambs and the two doors solid, each door's shape out as far as its inner
 * panel has slid */
static void elevator_place_shapes(struct task_work *work)
{
    struct collision_shape *shapes = work->collision->shapes;
    Float doors = DOORS(work);

    if (work->mode == DOORS_SHUT) {
        shapes[0].flags |= SHAPE_OFF;
        shapes[1].flags |= SHAPE_OFF;
        shapes[2].flags &= ~SHAPE_OFF;
        shapes[3].flags &= ~SHAPE_OFF;
        shapes[4].flags &= ~SHAPE_OFF;
    } else {
        shapes[0].flags &= ~SHAPE_OFF;
        shapes[1].flags &= ~SHAPE_OFF;
        shapes[2].flags |= SHAPE_OFF;
        shapes[3].flags &= ~SHAPE_OFF;
        shapes[4].flags &= ~SHAPE_OFF;
        doors *= 2.0F;
        shapes[0].center.x = DOOR_HALF + doors;
        shapes[1].center.x = -DOOR_HALF - doors;
    }
}

/* start the floor task, a child that puts the elevator's model in the world at
 * the elevator's position and heading */
static void elevator_floor_init(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    NJS_OBJECT *floor;

    force->turn.x = 0;
    force->turn.y = 0;
    force->turn.z = 0;
    force->speed.x = 0.0F;
    force->speed.y = 0.0F;
    force->speed.z = 0.0F;
    VARS(work)->object = floor = world_object_alloc();
    floor->evalflags = NJD_EVAL_UNIT_SCL | NJD_EVAL_BREAK | NJD_EVAL_ZXY_ANG;
    floor->scl[0] = 1.0F;
    floor->scl[1] = 1.0F;
    floor->scl[2] = 1.0F;
    floor->ang[0] = 0;
    floor->ang[1] = work->ang.y;
    floor->ang[2] = 0;
    floor->pos[0] = work->pos.x;
    floor->pos[1] = work->pos.y;
    floor->pos[2] = work->pos.z;
    floor->model = VARS(task->parent->work)->object->model;
    floor->child = NULL;
    floor->sibling = NULL;
    world_add(FLOOR_WORLD_FLAGS, task, floor);
    work->flags |= WORLD_TESTED;
    task->step = elevator_floor_step;
    task->destructor = elevator_floor_destroy;
}

static void elevator_floor_step(struct task *task)
{
}

static void elevator_floor_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->object) {
        world_remove(task, VARS(work)->object);
        world_object_free(VARS(work)->object);
    }
}

void corridor_step(struct task *task)
{
    if (!placement_editing(task)) {
        placement_kill(task);
    }
}

void twinkle_lights_start(void)
{
    if ((Uint8)stage_get_stage_act() == STATION_SQUARE_TWINKLE) {
        task_create(TASK_WORK, 3, twinkle_lights_init);
    }
}

static void twinkle_lights_init(struct task *task)
{
    struct task_work *work = task->work;

    LIGHTS_STAGE_ACT(work) = stage_get_stage_act();
    if ((Uint8)LIGHTS_STAGE_ACT(work) != STATION_SQUARE_TWINKLE) {
        task_kill(task);
        return;
    }
    work->ang.x = 0;
    work->ang.z = 0;
    LIT(work) = 0;
    LIGHTS_VARS(work)->steps = 0;
    task->step = twinkle_lights_step;
    task->draw = twinkle_lights_draw;
    task->destructor = twinkle_lights_destroy;
}

/* switch the lights between their two models as LIGHT_STEPS says, and end the
 * task once the stage or act changes */
static void twinkle_lights_step(struct task *task)
{
    struct task_work *work = task->work;

    if (LIGHTS_STAGE_ACT(work) != stage_get_stage_act()) {
        task_kill(task);
        return;
    }
    if (++LIGHTS_VARS(work)->steps < LIGHT_STEPS) {
        LIT(work) = 0;
    } else if (LIGHTS_VARS(work)->steps < 2 * LIGHT_STEPS) {
        LIT(work) = 1;
    } else {
        LIGHTS_VARS(work)->steps = 0;
    }
    twinkle_lights_draw(task);
}

/* draw each light LIGHT_SIDE to one side of its place, the side alternating
 * from light to light. the lit model starts on the other side from the unlit
 * one */
static void twinkle_lights_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;
    Float side;
    int i;

    njSetTexture(&ss_twins_texlist);
    if (LIT(work) == 0) {
        twinkle_light_object.model->mats[0].attr_texId =
            twinkle_light_textures[0];
        object = &twinkle_light_object;
        side = LIGHT_SIDE;
    } else {
        twinkle_light_lit_object.model->mats[0].attr_texId = LIT_TEXTURE;
        object = &twinkle_light_lit_object;
        side = -LIGHT_SIDE;
    }
    for (i = 0; i < LIGHTS; i++) {
        njPushMatrix(NULL);
        njTranslate(NULL, twinkle_light_places[i][0], LIGHT_Y,
                    twinkle_light_places[i][1]);
        if (i >= LIGHTS_ALONG_X) {
            njRotateY(NULL, 0x4000);
        }
        njTranslate(NULL, 0.0F, 0.0F, side);
        draw_tree(object);
        njPopMatrix(1);
        if (side == LIGHT_SIDE) {
            side = -LIGHT_SIDE;
        } else {
            side = LIGHT_SIDE;
        }
    }
}

static void twinkle_lights_destroy(struct task *task)
{
}
