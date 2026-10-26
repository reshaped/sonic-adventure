/* act 2's cargo ride: the train's run through the tunnel, 10000 units up */
#include <ninja.h>
#include <sg_xpt.h>

#include "object/switch.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "stg12/cargo_fade.h"
#include "stg12/cargo_ride.h"
#include "stg12/cargo_train.h"
#include "stg12/cargo_tunnel.h"
#include "stg12/cargo_world.h"
#include "system/pad.h"
#include "system/task.h"

/* the switch that brings the ride to its end. the ride turns it off as it
 * starts to slow */
#define END_SWITCH 16

/* a player within START_RANGE of the ride's center starts the ride. once
 * neither player is within LEAVE_RANGE of it, the ride ends without slowing */
#define START_RANGE 3000.0F
#define LEAVE_RANGE 2500.0F

/* the ride slows until its count of steps passes SLOW_STEPS. then it starts
 * the fade, and counts again from 0. once the count passes FADE_STEPS, the
 * ride moves player 0 off the train. the screen is black then */
#define SLOW_STEPS 120
#define FADE_STEPS 170

#define SE_TRAIN 325

enum cargo_ride_mode { RIDE_WAIT, RIDE_RUN, RIDE_SLOW, RIDE_FADE };

/* TASK_TIMER(work) holds the stage and act the ride's task was made for, as
 * STAGE_ACT packs them. the ride ends once they change */
struct cargo_ride_vars {
    Uint8 unk_00[4];
    int steps; /* the steps the ride has slowed, then those it has faded */
};

#define VARS(work) ((struct cargo_ride_vars *)&(work)->vars)

static void cargo_ride_init(struct task *task);
static void cargo_ride_step(struct task *task);

Bool cargo_ride_on = FALSE;
Bool cargo_ride_ending = FALSE;

static NJS_POINT3 cargo_ride_center = {0.0F, 10000.0F, 1720.0F};

void cargo_ride_start(void)
{
    cargo_ride_on = FALSE;
    switch_set(END_SWITCH, FALSE);
    cargo_ride_ending = FALSE;
    task_create(TASK_WORK, 3, cargo_ride_init);
}

/* cargo_ride_step is the task's destructor too, so it runs once more as the
 * task is destroyed */
static void cargo_ride_init(struct task *task)
{
    struct task_work *work = task->work;

    work->mode = RIDE_WAIT;
    TASK_TIMER(work) = stage_get_stage_act();
    task->step = cargo_ride_step;
    task->destructor = cargo_ride_step;
}

static void cargo_ride_step(struct task *task)
{
    struct task_work *work = task->work;

    if (TASK_TIMER(work) != stage_get_stage_act()) {
        cargo_ride_on = FALSE;
        task_kill(task);
    }
    switch (work->mode) {
    case RIDE_WAIT:
        if (player_in_range(&cargo_ride_center, START_RANGE)) {
            cargo_ride_on = TRUE;
            work->mode = RIDE_RUN;
            task_create(TASK_WORK, 3, cargo_tunnel_start);
            cargo_world_start();
            task_create(TASK_WORK, 3, cargo_train_start);
        }
        break;
    case RIDE_RUN:
        if (!player_in_range(&cargo_ride_center, LEAVE_RANGE)) {
            cargo_ride_on = FALSE;
            task_kill(task);
        }
        if (switch_is_on(END_SWITCH)) {
            switch_set(END_SWITCH, FALSE);
            cargo_ride_ending = TRUE;
            VARS(work)->steps = 0;
            work->mode = RIDE_SLOW;
        }
        break;
    case RIDE_SLOW:
        ++VARS(work)->steps;
        sound_play_for(SE_TRAIN, (int)task, 1, 0, 2);
        if (VARS(work)->steps > SLOW_STEPS) {
            VARS(work)->steps = 0;
            work->mode = RIDE_FADE;
            if (pad_is_enabled()) {
                pad_disable();
            }
            task_create(TASK_WORK, 3, cargo_fade_start);
        }
        break;
    case RIDE_FADE:
        ++VARS(work)->steps;
        sound_play_for(SE_TRAIN, (int)task, 1, 0, 2);
        if (VARS(work)->steps > FADE_STEPS) {
            if (!pad_is_enabled()) {
                pad_enable();
            }
            player_set_position(0, -850.0F, 3030.0F, -3183.0F);
            cargo_ride_on = FALSE;
            task_kill(task);
        }
        break;
    }
}
