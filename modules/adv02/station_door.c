/* door below the station, whose two panels slide apart and back together as
 * FLAG_STATION_DOOR says */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv02/station_door.h"
#include "adv02/texlists.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "progress/progress.h"
#include "system/task.h"

/* @hack type: the four objects declared as arrays, and read through ->. SHC
 * then loads each object's address and reads its fields past it, as the door's
 * model at 0x0c90bd0e (MOV.L @(4,R4),R4). as NJS_OBJECTs read through ., 63 of
 * station_door_draw's 90 words differ */
extern NJS_OBJECT station_door_object[];
extern NJS_OBJECT station_door_middle_object[];
extern NJS_OBJECT station_door_left_object[];
extern NJS_OBJECT station_door_right_object[];

#define DOOR_X 441.776F
#define DOOR_Y (-319.1976F)
#define DOOR_Z 896.727F

/* how far each panel slides out from shut */
#define OPEN_TRAVEL 40.0F
/* a moving panel speeds up by SPEED_UP a step, up to TOP_SPEED. once it
 * brakes, it slows down by SLOW_DOWN a step, to no less than LOW_SPEED */
#define SPEED_UP 0.03F
#define TOP_SPEED 0.5F
#define SLOW_DOWN 0.06F
#define LOW_SPEED 0.05F
/* an opening panel brakes once it is OPENING_BRAKE of OPEN_TRAVEL out from
 * shut, and a shutting one once it is SHUTTING_BRAKE out */
#define OPENING_BRAKE 0.6F
#define SHUTTING_BRAKE 0.4F

enum station_door_mode { DOOR_INIT, DOOR_RUN };

struct station_door_vars {
    Float speed; /* the panels' speed a step, below 0 while they shut */
    Float travel; /* how far each panel has slid out from shut */
};

#define VARS(work) ((struct station_door_vars *)&(work)->vars)

static void station_door_init(struct task *task);
static void station_door_update(struct task *task);
static void station_door_draw(struct task *task);
static void station_door_destroy(struct task *task);

void station_door_step(struct task *task)
{
    struct task_work *work = task->work;

    /* the step runs the same whether or not the set editor holds the task */
    if (!placement_editing(task)) {
        switch (work->mode) {
        case DOOR_INIT:
            station_door_init(task);
            break;
        case DOOR_RUN:
            station_door_draw(task);
            station_door_update(task);
            break;
        }
    } else {
        switch (work->mode) {
        case DOOR_INIT:
            station_door_init(task);
            break;
        case DOOR_RUN:
            station_door_draw(task);
            station_door_update(task);
            break;
        }
    }
}

static void station_door_init(struct task *task)
{
    struct task_work *work = task->work;

    work->pos.x = DOOR_X;
    work->pos.y = DOOR_Y;
    work->pos.z = DOOR_Z;
    task->draw = station_door_draw;
    task->destructor = station_door_destroy;
    VARS(work)->travel = 0.0F;
    VARS(work)->speed = 0.0F;
    work->mode = DOOR_RUN;
}

static void station_door_update(struct task *task)
{
    struct task_work *work = task->work;

    switch (event_flags[FLAG_STATION_DOOR]) {
    case STATION_DOOR_SHUT:
        VARS(work)->travel = 0.0F;
        VARS(work)->speed = 0.0F;
        break;
    case STATION_DOOR_SHUTTING:
        if (VARS(work)->travel > OPEN_TRAVEL * SHUTTING_BRAKE) {
            VARS(work)->speed -= SPEED_UP;
            if (VARS(work)->speed < -TOP_SPEED) {
                VARS(work)->speed = -TOP_SPEED;
            }
        } else {
            VARS(work)->speed += SLOW_DOWN;
            if (VARS(work)->speed > -LOW_SPEED) {
                VARS(work)->speed = -LOW_SPEED;
            }
        }
        break;
    case STATION_DOOR_OPEN:
        VARS(work)->travel = OPEN_TRAVEL;
        VARS(work)->speed = 0.0F;
        break;
    case STATION_DOOR_OPENING:
        if (VARS(work)->travel < OPEN_TRAVEL * OPENING_BRAKE) {
            VARS(work)->speed += SPEED_UP;
            if (VARS(work)->speed > TOP_SPEED) {
                VARS(work)->speed = TOP_SPEED;
            }
        } else {
            VARS(work)->speed -= SLOW_DOWN;
            if (VARS(work)->speed < LOW_SPEED) {
                VARS(work)->speed = LOW_SPEED;
            }
        }
        break;
    }
    VARS(work)->travel += VARS(work)->speed;
    if (VARS(work)->travel < 0.0F) {
        VARS(work)->travel = 0.0F;
    } else if (VARS(work)->travel > OPEN_TRAVEL) {
        VARS(work)->travel = OPEN_TRAVEL;
    }
}

static void station_door_draw(struct task *task)
{
    struct task_work *work = task->work;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njSetTexture(&mrobj_texlist);
    draw_model(station_door_object->model);
    njPushMatrix(NULL);
    njTranslate(NULL, station_door_middle_object->pos[0],
                station_door_middle_object->pos[1],
                station_door_middle_object->pos[2]);
    draw_model(station_door_middle_object->model);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslate(NULL, station_door_left_object->pos[0] - VARS(work)->travel,
                station_door_left_object->pos[1],
                station_door_left_object->pos[2]);
    draw_model(station_door_left_object->model);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslate(NULL, station_door_right_object->pos[0] + VARS(work)->travel,
                station_door_right_object->pos[1],
                station_door_right_object->pos[2]);
    draw_model(station_door_right_object->model);
    njPopMatrix(1);
    njPopMatrix(1);
}

static void station_door_destroy(struct task *task)
{
}
