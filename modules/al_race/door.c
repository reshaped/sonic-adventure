/* the race hall's door, which takes a player through it to the Station
 * Square garden */
#include <ninja.h>
#include <sg_xpt.h>

#include "al_race/door.h"
#include "al_race/scenery.h"
#include "collision/collision.h"
#include "draw/draw.h"
#include "object/placement.h"
#include "object/simple.h"
#include "player/reset.h"
#include "sound/cue.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/task_desc.h"
#include "system/texture.h"

/* the door's model and its opening */
extern NJS_ACTION dat_0cbd22fc;

/* the door's state, in its task's motion */
struct door {
    Sint8 mode;
    Uint8 unk_01;
    Sint16 timer; /* the steps left in DOOR_TAKE_WAIT */
    Float frame;
    Float last; /* the action's last frame */
    Float speed; /* added to frame each step, negative to shut */
    NJS_ACTION *action;
};

#define DOOR(task) ((struct door *)(task)->motion)

/* the door's modes. something in the doorway holds player 0 and sends the
 * door through the DOOR_TAKE modes. they open the door fully and shut it,
 * then fade to the garden after DOOR_WAIT steps. DOOR_GONE waits out the
 * fade */
enum door_mode {
    DOOR_SHUT,
    DOOR_OPENING,
    DOOR_OPEN,
    DOOR_SHUTTING,
    DOOR_TAKE_OPENING,
    DOOR_TAKE_SHUTTING,
    DOOR_TAKE_WAIT,
    DOOR_GONE
};

/* what the door's shapes touch: nothing, something in front of the door, or
 * something in the doorway */
enum door_touch { TOUCH_NONE, TOUCH_NEAR, TOUCH_IN };

/* the frames the door's action plays a step. the action runs from frame 0
 * to its last, frame 1. SHC folds the quotient to 0x3d088888 (0x0cb97528),
 * one below 1/30's 0x3d088889, so the door opens in 31 steps. it shuts in
 * 30, as the game's FPSCR rounds each sum toward zero */
#define DOOR_SPEED (1.0F / 30.0F)

#define SE_DOOR 0x273
/* the steps the door waits, shut behind a player, before the fade */
#define DOOR_WAIT 4

/* the Station Square garden's entrance from the race, where the door takes the
 * player */
#define GARDEN_ENTRANCE 2

static void door_start(struct task *task, void *arg);
static void door_step(struct task *task);
static void door_draw(struct task *task);
static enum door_touch door_touched(struct task_work *work);

static const struct task_desc door_desc = {
    sizeof(struct door), TASK_WORK, door_start, door_step, door_draw, NULL};
static NJS_ACTION *const door_action = &dat_0cbd22fc;
static struct collision_shape door_shapes[2];
static const struct collision_list door_list = {door_shapes, 2, 40.0F};
static const struct collision_kind door_collision = {COLLISION_OBJECT, 1};

/* door_opening's result, which only a start from DOOR_SHUT sets */
static Sint8 door_started = 0;
static struct collision_shape door_shapes[2] = {
    {SHAPE_RECTANGLE,
     0,
     PUSH_FIRM(PUSH_ALL),
     {-15.0F, 13.0F, 0.0F},
     15.0F,
     13.0F,
     20.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_FIRM(PUSH_ALL),
     {15.0F, 13.0F, 0.0F},
     15.0F,
     13.0F,
     20.0F,
     {0, 0, 0},
     0,
     NULL},
};

void door_create(void)
{
    scenery_add(&door_desc, (void *)&door_action);
}

Bool door_opening(void)
{
    return door_started;
}

static void door_start(struct task *task, void *arg)
{
    struct task_work *work = task->work;
    struct door *door = DOOR(task);
    NJS_ACTION *action = *(NJS_ACTION **)arg;

    scenery_place(work, action->object);
    object_collision_init(task, (struct collision_list *)&door_list,
                          (struct collision_kind *)&door_collision);
    door->mode = DOOR_SHUT;
    door->timer = 0;
    door->frame = 0.0F;
    door->last = action->motion->nbFrame - 1.0F;
    door->speed = DOOR_SPEED;
    door->action = action;
    door_started = FALSE;
}

static void door_step(struct task *task)
{
    struct door *door = DOOR(task);
    struct task_work *work = task->work;
    enum door_touch touch = door_touched(work);
    Bool done;

    switch (door->mode) {
    case DOOR_SHUT:
        switch (touch) {
        case TOUCH_NONE:
            break;
        case TOUCH_NEAR:
            door->speed = DOOR_SPEED;
            door->mode = DOOR_OPENING;
            door_started = TRUE;
            sound_play_near(SE_DOOR, &work->pos, NULL);
            break;
        case TOUCH_IN:
            player_hold(0);
            door->speed = DOOR_SPEED;
            door->mode = DOOR_TAKE_OPENING;
            door_started = TRUE;
            sound_play_near(SE_DOOR, &work->pos, NULL);
            break;
        }
        break;
    case DOOR_OPENING:
        done = FALSE;
        door->frame += door->speed;
        if (door->frame >= door->last) {
            door->frame = door->last;
            done = TRUE;
        }
        switch (touch) {
        case TOUCH_NONE:
            if (done) {
                door->speed = -DOOR_SPEED;
                door->mode = DOOR_SHUTTING;
            }
            break;
        case TOUCH_NEAR:
        default:
            if (done) {
                door->mode = DOOR_OPEN;
            }
            break;
        case TOUCH_IN:
            player_hold(0);
            if (done) {
                door->speed = -DOOR_SPEED;
                door->mode = DOOR_TAKE_SHUTTING;
            } else {
                door->speed = DOOR_SPEED;
                door->mode = DOOR_TAKE_OPENING;
            }
            break;
        }
        door_started = FALSE;
        break;
    case DOOR_OPEN:
        switch (touch) {
        case TOUCH_NONE:
            door->speed = -DOOR_SPEED;
            door->mode = DOOR_SHUTTING;
            break;
        case TOUCH_NEAR:
            break;
        case TOUCH_IN:
            player_hold(0);
            door->speed = -DOOR_SPEED;
            door->mode = DOOR_TAKE_SHUTTING;
            break;
        }
        break;
    case DOOR_SHUTTING:
        done = FALSE;
        door->frame += door->speed;
        if (door->frame <= 0.0F) {
            door->frame = 0.0F;
            done = TRUE;
        }
        switch (touch) {
        case TOUCH_NONE:
            if (done) {
                door->mode = DOOR_SHUT;
            }
            break;
        case TOUCH_NEAR:
        default:
            door->speed = DOOR_SPEED;
            door->mode = DOOR_OPENING;
            break;
        case TOUCH_IN:
            player_hold(0);
            door->speed = DOOR_SPEED;
            door->mode = DOOR_TAKE_OPENING;
            break;
        }
        break;
    case DOOR_TAKE_OPENING:
        door->frame += door->speed;
        if (door->frame >= door->last) {
            door->frame = door->last;
            door->speed = -DOOR_SPEED;
            door->mode = DOOR_TAKE_SHUTTING;
        }
        door_started = FALSE;
        break;
    case DOOR_TAKE_SHUTTING:
        door->frame += door->speed;
        if (door->frame <= 0.0F) {
            door->frame = 0.0F;
            door->timer = DOOR_WAIT;
            door->mode = DOOR_TAKE_WAIT;
        }
        break;
    case DOOR_TAKE_WAIT:
        if (door->timer > 0) {
            --door->timer;
        } else {
            stage_set_entrance(GARDEN_ENTRANCE);
            stage_fade_to(STAGE_ALIFE_SS, 0);
            door->mode = DOOR_GONE;
        }
        break;
    case DOOR_GONE:
        break;
    }
    collision_register(work);
    door_draw(task);
}

static void door_draw(struct task *task)
{
    if (frame_step == 0 && !placement_far_from_camera(task)) {
        struct door *door = DOOR(task);

        texture_set_stage();
        draw_action(door->action, door->frame);
    }
}

/* shape 0 is in front of the door and shape 1 in the doorway; the first takes
 * precedence */
static enum door_touch door_touched(struct task_work *work)
{
    struct collision_shape *shapes = work->collision->shapes;
    enum door_touch touch;

    if (shapes[0].hit == NULL) {
        if (shapes[1].hit == NULL) {
            touch = TOUCH_NONE;
        } else {
            touch = TOUCH_IN;
        }
    } else {
        touch = TOUCH_NEAR;
    }
    return touch;
}
