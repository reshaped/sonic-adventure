/* O Elevator1 car with doors that rides each act's shaft with player 0 */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "object/switch.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stage/scene_changer.h"
#include "stage/stage.h"
#include "stg10/elevator1.h"
#include "system/frame.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_ACTION elevator1_action;
extern NJS_OBJECT elevator1_door_object;
extern NJS_OBJECT elevator1_world_object;

/* destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)

/* in act 1, the shut doors open once a player comes this near */
#define OPEN_NEAR 80.0F
/* the collision world tests the car while a player is this near */
#define NEAR 60.0F

#define RIDE_SPEED 0.6F /* units a step */
#define ACTION_FRAMES 60.0F /* elevator1_action's length */

/* the heights of each act's shaft. the car starts at its act's top, and a
 * ride ends at the top or the bottom */
#define ACT_0_TOP 154.0F
#define ACT_0_BOTTOM (-239.0F)
#define ACT_1_TOP (-1624.85F)
#define ACT_1_BOTTOM (-1858.0F)
#define ACT_2_TOP 230.0F
#define ACT_2_BOTTOM (-2.0F)

/* DOORS(work) with the doors open and with them shut */
#define DOORS_OPEN 0.06F
#define DOORS_SHUT 1.0F
/* DOORS(work)'s change each step as the doors open and as they shut */
#define OPEN_SPEED 0.02F
#define SHUT_SPEED 0.01F
/* while the doors shut, elevator1_shape blocks the doorway once DOORS(work)
 * passes this */
#define DOORS_BLOCK 0.2F

/* in act 0, the car leaves the top of its shaft once this switch is on */
#define CALL_SWITCH 2
/* where act 1's ride goes at the bottom of its shaft: act 2, entrance 0 */
#define ACT_1_DESTINATION STAGE_ACT(STAGE_FINAL, 2)

#define SE_ELEVATOR1_RIDE 300
#define SE_ELEVATOR1_DOORS 291

enum elevator1_mode {
    CAR_INIT,
    CAR_SHUT, /* the car waits for a player within OPEN_NEAR */
    CAR_OPENING,
    CAR_OPEN, /* the car waits for player 0 to stand in it */
    CAR_SHUTTING,
    CAR_UP,
    CAR_DOWN,
    CAR_SWITCH, /* the car waits for CALL_SWITCH to turn on */
    CAR_STOPPED
};

struct elevator1_vars {
    Float frame; /* the frame of elevator1_action the car shows */
    Uint8 unk_04[4];
    NJS_OBJECT *car; /* the car's object in the collision world */
};

#define VARS(work) ((struct elevator1_vars *)&(work)->vars)
/* its speed carries what stands in the car */
#define FORCE(task) ((struct task_force *)(task)->force)
#define DOORS(work) ((work)->scl.x) /* the doors' x scale */
/* the mode the car rides in once its doors shut on player 0: CAR_UP or
 * CAR_DOWN. RIDE holds CAR_STOPPED when no ride is left. the car then stays
 * in CAR_STOPPED once its doors open, rather than wait for a rider */
#define RIDE(work) ((work)->submode)

static void elevator1_init(struct task *task);
static void elevator1_update(struct task *task);
static void elevator1_draw(struct task *task);
static void elevator1_up(struct task *task);
static void elevator1_down(struct task *task);
static void elevator1_wait_switch(struct task *task);
static void elevator1_add(struct task *task);
static void elevator1_destroy(struct task *task);
static void elevator1_wait_rider(struct task *task);
static void elevator1_open(struct task *task);
static void elevator1_shut(struct task *task);
static void elevator1_wait_near(struct task *task);

/* a box across the doorway, where the doors shut */
static struct collision_shape elevator1_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 20.0F, 16.0F},
    15.0F,
    17.5F,
    2.5F,
    {0},
    0,
    NULL,
};

/* outside the set editor, the car starts at the top of its act's shaft. the
 * step overwrites the place and angles its set entry gives */
void elevator1_step(struct task *task)
{
    struct task_work *work = task->work;
    Uint16 stage_act;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case CAR_INIT:
            elevator1_init(task);
            stage_act = stage_get_stage_act();
            switch ((Uint8)stage_act) {
            case 0:
                work->pos.x = 1271.0F;
                work->pos.y = ACT_0_TOP;
                work->pos.z = -1457.0F;
                work->ang.x = 0;
                work->ang.y = 0x8000;
                work->ang.z = 0;
                work->mode = CAR_SWITCH;
                RIDE(work) = CAR_UP;
                break;
            case 1:
                work->pos.x = 1061.34F;
                work->pos.y = ACT_1_TOP;
                work->pos.z = -850.0F;
                work->ang.x = 0;
                work->ang.y = 0x8000;
                work->ang.z = 0;
                work->mode = CAR_SHUT;
                RIDE(work) = CAR_DOWN;
                break;
            case 2:
                work->pos.x = 1071.7F;
                work->pos.y = ACT_2_TOP;
                work->pos.z = -518.0F;
                work->ang.x = 0;
                work->ang.y = 0x8000;
                work->ang.z = 0;
                work->mode = CAR_DOWN;
                RIDE(work) = CAR_STOPPED;
                pad_disable();
                break;
            default:
                work->mode = CAR_STOPPED;
                break;
            }
            break;
        case CAR_SHUT:
            elevator1_draw(task);
            elevator1_wait_near(task);
            elevator1_update(task);
            collision_register(work);
            break;
        case CAR_OPENING:
            elevator1_draw(task);
            elevator1_open(task);
            elevator1_update(task);
            break;
        case CAR_OPEN:
            elevator1_draw(task);
            elevator1_wait_rider(task);
            elevator1_update(task);
            break;
        case CAR_SHUTTING:
            elevator1_draw(task);
            elevator1_shut(task);
            elevator1_update(task);
            if (DOORS(work) > DOORS_BLOCK) {
                collision_register(work);
            }
            break;
        case CAR_UP:
            elevator1_draw(task);
            elevator1_up(task);
            elevator1_update(task);
            collision_register(work);
            break;
        case CAR_DOWN:
            elevator1_draw(task);
            elevator1_down(task);
            elevator1_update(task);
            collision_register(work);
            break;
        case CAR_SWITCH:
            elevator1_draw(task);
            elevator1_wait_switch(task);
            elevator1_update(task);
            break;
        case CAR_STOPPED:
            elevator1_draw(task);
            elevator1_update(task);
            break;
        }
    } else {
        switch (work->mode) {
        case CAR_INIT:
            elevator1_init(task);
            work->mode = CAR_SHUT;
            break;
        case CAR_SHUT:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> ROTATE X");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> ROTATE Z");
            elevator1_draw(task);
            elevator1_update(task);
            break;
        }
    }
}

static void elevator1_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = elevator1_draw;
    task->destructor = elevator1_destroy;
    elevator1_add(task);
    VARS(work)->frame = 0.0F;
}

static void elevator1_update(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *car;
    NJS_OBJECT *from;

    if (player_in_range(&work->pos, NEAR)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
    from = &elevator1_world_object;
    car = VARS(work)->car;
    car->ang[0] = 0;
    car->ang[1] = work->ang.y;
    car->ang[2] = 0;
    car->pos[0] = work->pos.x + from->pos[0];
    car->pos[1] = work->pos.y + from->pos[1];
    car->pos[2] = work->pos.z + from->pos[2];
}

static void elevator1_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_action(&elevator1_action, VARS(work)->frame);
        njPushMatrix(NULL);
        njTranslate(NULL, 10.0F, 20.0F, 16.0F);
        njScale(NULL, DOORS(work), 1.0F, 1.0F);
        draw_tree(&elevator1_door_object);
        njPopMatrix(1);
        njPushMatrix(NULL);
        njTranslate(NULL, 10.0F, 20.0F, 13.0F);
        njScale(NULL, DOORS(work), 1.0F, 1.0F);
        draw_tree(&elevator1_door_object);
        njPopMatrix(1);
        njPopMatrix(1);
    }
}

static void elevator1_up(struct task *task)
{
    struct task_work *work = task->work;

    work->pos.y += RIDE_SPEED;
    FORCE(task)->speed.y = RIDE_SPEED;
    VARS(work)->frame += 1.0F;
    if (VARS(work)->frame == ACTION_FRAMES / 2) {
        sound_play(SE_ELEVATOR1_RIDE, 0, 0, 0);
    }
    if (VARS(work)->frame >= ACTION_FRAMES) {
        sound_play(SE_ELEVATOR1_RIDE, 0, 0, 0);
        VARS(work)->frame = 0.0F;
    }
    if (work->pos.y >= ACT_0_TOP) {
        FORCE(task)->speed.y = 0.0F;
        work->mode = CAR_OPENING;
        RIDE(work) = CAR_STOPPED;
        sound_play(SE_ELEVATOR1_DOORS, 0, 0, 0);
        pad_enable();
    }
}

static void elevator1_down(struct task *task)
{
    struct task_work *work = task->work;
    Uint16 stage_act;

    work->pos.y -= RIDE_SPEED;
    FORCE(task)->speed.y = -RIDE_SPEED;
    VARS(work)->frame -= 1.0F;
    if (VARS(work)->frame == ACTION_FRAMES / 2) {
        sound_play(SE_ELEVATOR1_RIDE, 0, 0, 0);
    }
    if (VARS(work)->frame < 0.0F) {
        sound_play(SE_ELEVATOR1_RIDE, 0, 0, 0);
        VARS(work)->frame = ACTION_FRAMES - 1.0F;
    }
    stage_act = stage_get_stage_act();
    switch ((Uint8)stage_act) {
    case 0:
        if (work->pos.y <= ACT_0_BOTTOM) {
            FORCE(task)->speed.y = 0.0F;
            work->mode = CAR_OPENING;
            sound_play(SE_ELEVATOR1_DOORS, 0, 0, 0);
            player_event_end(0);
            pad_enable();
        }
        break;
    case 1:
        if (work->pos.y <= ACT_1_BOTTOM) {
            scene_changer_go(ACT_1_DESTINATION);
        }
        break;
    case 2:
        if (work->pos.y <= ACT_2_BOTTOM) {
            FORCE(task)->speed.y = 0.0F;
            work->mode = CAR_OPENING;
            sound_play(SE_ELEVATOR1_DOORS, 0, 0, 0);
            player_event_end(0);
            pad_enable();
        }
        break;
    }
}

static void elevator1_wait_switch(struct task *task)
{
    struct task_work *work = task->work;

    if (switch_is_on(CALL_SWITCH)) {
        work->mode = CAR_DOWN;
    }
}

static void elevator1_add(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *from;
    NJS_OBJECT *car;

    DOORS(work) = DOORS_SHUT;
    from = &elevator1_world_object;
    VARS(work)->car = car = world_object_alloc();
    car->evalflags = from->evalflags;
    car->scl[0] = 1.0F;
    car->scl[1] = 1.0F;
    car->scl[2] = 1.0F;
    car->ang[0] = 0;
    car->ang[1] = work->ang.y;
    car->ang[2] = 0;
    car->pos[0] = work->pos.x + from->pos[0];
    car->pos[1] = work->pos.y + from->pos[1];
    car->pos[2] = work->pos.z + from->pos[2];
    car->model = from->model;
    car->child = NULL;
    car->sibling = NULL;
    world_add(WORLD_MOVES, task, car);
    collision_init(task, &elevator1_shape, COLLISION_OBJECT, 1, 1);
}

static void elevator1_destroy(struct task *task)
{
    struct task_work *work = task->work;

    world_remove(task, VARS(work)->car);
    world_object_free(VARS(work)->car);
}

static void elevator1_wait_rider(struct task *task)
{
    struct task_work *work = task->work;

    /* @hack redundant: player 0 tested twice, as the image tests it
     * (MOV #0,R4 at 0x0c912470 and 0x0c91247c). player_stands_on changes
     * nothing, so the second test repeats the first; tested once, 33 of 34
     * words differ */
    /* NOLINTNEXTLINE(misc-redundant-expression) */
    if (player_stands_on(0, task) || player_stands_on(0, task)) {
        work->mode = CAR_SHUTTING;
        work->timer = 300; /* the car never reads its timer */
        sound_play(SE_ELEVATOR1_DOORS, 0, 0, 0);
    }
}

static void elevator1_open(struct task *task)
{
    struct task_work *work = task->work;

    DOORS(work) -= OPEN_SPEED;
    if (DOORS(work) < DOORS_OPEN) {
        DOORS(work) = DOORS_OPEN;
        if (RIDE(work) == CAR_STOPPED) {
            work->mode = CAR_STOPPED;
        } else {
            work->mode = CAR_OPEN;
        }
    }
}

static void elevator1_shut(struct task *task)
{
    struct task_work *work = task->work;

    DOORS(work) += SHUT_SPEED;
    if (DOORS(work) > DOORS_SHUT) {
        DOORS(work) = DOORS_SHUT;
        /* @hack redundant: player 0 tested twice, as the image tests it
         * (MOV #0,R4 at 0x0c912526 and 0x0c912532). player_stands_on
         * changes nothing, so the second test repeats the first; tested
         * once, elevator1.c's code runs 32 bytes short and 53 of 57 words
         * differ */
        /* NOLINTNEXTLINE(misc-redundant-expression) */
        if (player_stands_on(0, task) || player_stands_on(0, task)) {
            pad_disable();
            work->mode = RIDE(work);
            sound_play(SE_ELEVATOR1_RIDE, 0, 0, 0);
        } else {
            work->mode = CAR_OPENING;
            sound_play(SE_ELEVATOR1_DOORS, 0, 0, 0);
        }
    }
}

static void elevator1_wait_near(struct task *task)
{
    struct task_work *work = task->work;

    if (player_in_range(&work->pos, OPEN_NEAR)) {
        work->mode = CAR_OPENING;
    }
}
