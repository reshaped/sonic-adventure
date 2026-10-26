/* the cargo ride's train, which runs back and forth along z between two
 * stops */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "stage/stage.h"
#include "stg12/cargo_car.h"
#include "stg12/cargo_container.h"
#include "stg12/cargo_ride.h"
#include "stg12/cargo_top.h"
#include "stg12/cargo_train.h"
#include "stg12/connection.h"
#include "system/task.h"

/* the train's parts: its cars, couplings and containers */
#define PARTS 22
/* each car and coupling stands this far along z from the one ahead */
#define CAR_SPACING (-172.0F)

#define TOP_SPEED 5.0F
#define ACCEL 0.1F /* the speed the train gains or loses each step */

/* where the train starts */
#define TRAIN_X 55.0F
#define TRAIN_Y 9990.0F
#define START_Z (-500.0F)

/* the train brakes to a stop near each of these z, and waits WAIT_STEPS
 * there */
#define STOP_NEAR 2924.0F
#define STOP_FAR 3440.0F
#define WAIT_STEPS 400

enum cargo_train_part {
    PART_CAR,
    PART_TOP,
    PART_CONNECTION,
    PART_CONTAINER_A,
    PART_CONTAINER_B
};

/* in TRAIN_WAIT, work->submode holds the mode to resume */
enum cargo_train_mode {
    TRAIN_SPEED_UP,
    TRAIN_RUN,
    TRAIN_BRAKE,
    TRAIN_WAIT,
    TRAIN_BACK_UP,
    TRAIN_RUN_BACK,
    TRAIN_BRAKE_BACK
};

/* TASK_TIMER(work) holds the stage and act the train started in, which nothing
 * reads */
struct cargo_train_vars {
    Uint32 wait; /* the steps left in TRAIN_WAIT */
    Uint8 unk_04[4];
    Float speed;
};

#define VARS(work) ((struct cargo_train_vars *)&(work)->vars)
/* the train's braking distance from TOP_SPEED, and the z at which it starts to
 * brake */
#define BRAKE_DISTANCE(work) ((work)->scl.x)
#define BRAKE_Z(work) ((work)->scl.y)

static void cargo_train_step(struct task *task);
static void cargo_train_destroy(struct task *task);

/* the train's parts, front to back. a container takes the z of the car after
 * it, which it rides */
static enum cargo_train_part cargo_train_parts[PARTS] = {
    PART_TOP,         PART_CONTAINER_A, PART_CAR,        PART_CONTAINER_B,
    PART_CAR,         PART_CONTAINER_A, PART_CAR,        PART_CONTAINER_B,
    PART_CAR,         PART_CONTAINER_A, PART_CAR,        PART_CONTAINER_A,
    PART_CAR,         PART_CONNECTION,  PART_CONNECTION, PART_CONNECTION,
    PART_CAR,         PART_CAR,         PART_CAR,        PART_CAR,
    PART_CONTAINER_A, PART_CAR,
};

Float cargo_train_speed;

void cargo_train_start(struct task *task)
{
    struct task_work *work = task->work;
    struct task *part;
    struct task_work *part_work;
    Float steps;
    Float z;
    int i;

    work->pos.y = TRAIN_Y;
    work->pos.x = TRAIN_X;
    work->pos.z = START_Z;
    work->submode = TRAIN_SPEED_UP;
    TASK_TIMER(work) = stage_get_stage_act();
    /* the first stop sets wait to WAIT_STEPS before TRAIN_WAIT reads it, so
     * no step reads this 200 */
    VARS(work)->wait = 200;
    VARS(work)->speed = TOP_SPEED;
    cargo_train_speed = TOP_SPEED;
    work->mode = TRAIN_RUN;
    steps = ceilf(TOP_SPEED / ACCEL);
    BRAKE_DISTANCE(work) = (steps * TOP_SPEED) - (steps * steps * (ACCEL / 2));
    BRAKE_Z(work) = STOP_NEAR - BRAKE_DISTANCE(work);
    z = work->pos.z;
    for (i = 0; i < PARTS; i++) {
        switch (cargo_train_parts[i]) {
        case PART_CAR:
            part = task_create_child(TASK_WORK | TASK_FORCE,
                                     cargo_car_train_step, task);
            part_work = part->work;
            part_work->pos.z = z;
            z += CAR_SPACING;
            break;
        case PART_TOP:
            part = task_create_child(TASK_WORK | TASK_FORCE,
                                     cargo_top_train_step, task);
            part_work = part->work;
            part_work->pos.z = z + (CAR_SPACING / 2);
            z += CAR_SPACING;
            break;
        case PART_CONNECTION:
            part = task_create_child(TASK_WORK, connection_train_step, task);
            part_work = part->work;
            part_work->pos.z = z;
            z += CAR_SPACING;
            break;
        /* cargo_container_init reads scale x: 0 for A, 1 for B */
        case PART_CONTAINER_A:
            part = task_create_child(TASK_WORK | TASK_FORCE,
                                     cargo_container_train_step, task);
            part_work = part->work;
            part_work->pos.z = z;
            part_work->scl.x = 0.0F;
            break;
        case PART_CONTAINER_B:
            part = task_create_child(TASK_WORK | TASK_FORCE,
                                     cargo_container_train_step, task);
            part_work = part->work;
            part_work->pos.z = z;
            part_work->scl.x = 1.0F;
            break;
        }
    }
    task->step = cargo_train_step;
    task->destructor = cargo_train_destroy;
}

/* the train starts at full speed and stops near STOP_NEAR. from then on it
 * shuttles to STOP_FAR and back, waiting WAIT_STEPS at each stop */
static void cargo_train_step(struct task *task)
{
    struct task_work *work = task->work;
    Float speed;

    if (!cargo_ride_on) {
        task_kill(task);
        return;
    }
    switch (work->mode) {
    case TRAIN_SPEED_UP:
        speed = VARS(work)->speed + ACCEL;
        if (speed >= TOP_SPEED) {
            BRAKE_Z(work) = STOP_FAR - BRAKE_DISTANCE(work);
            work->mode = TRAIN_RUN;
            speed = TOP_SPEED;
        }
        VARS(work)->speed = speed;
        cargo_train_speed = speed;
        work->pos.z += speed;
        break;
    case TRAIN_RUN:
        work->pos.z += VARS(work)->speed;
        if (work->pos.z >= BRAKE_Z(work)) {
            work->mode = TRAIN_BRAKE;
        }
        break;
    case TRAIN_BRAKE:
        /* @hack spelling: the speed loaded in a statement of its own, then less
         * ACCEL. SHC subtracts FR5, which holds ACCEL through the step, FSUB
         * FR5,FR4 at 0x0c912126. as one statement, it adds -0.1 from the pool,
         * and 48 of 164 words differ */
        speed = VARS(work)->speed;
        speed -= ACCEL;
        if (speed <= 0.0F) {
            VARS(work)->wait = WAIT_STEPS;
            work->mode = TRAIN_WAIT;
            speed = 0.0F;
            if (BRAKE_Z(work) < STOP_NEAR) {
                work->submode = TRAIN_SPEED_UP;
            } else {
                work->submode = TRAIN_BACK_UP;
            }
        }
        VARS(work)->speed = speed;
        cargo_train_speed = speed;
        work->pos.z += speed;
        break;
    case TRAIN_WAIT:
        /* wait is unsigned, so the test holds only when it reaches 0 */
        if (--VARS(work)->wait <= 0) {
            VARS(work)->wait = 0;
            work->mode = work->submode;
        }
        break;
    case TRAIN_BACK_UP:
        /* @hack spelling: as in TRAIN_BRAKE, the speed loaded in a statement of
         * its own, then less ACCEL. SHC subtracts FR5, FSUB FR5,FR4 at
         * 0x0c912162. as one statement, it adds -0.1 from the pool, and 16 of
         * 164 words differ */
        speed = VARS(work)->speed;
        speed -= ACCEL;
        if (speed <= -TOP_SPEED) {
            BRAKE_Z(work) = BRAKE_DISTANCE(work) + STOP_NEAR;
            work->mode = TRAIN_RUN_BACK;
            speed = -TOP_SPEED;
        }
        VARS(work)->speed = speed;
        cargo_train_speed = speed;
        work->pos.z += speed;
        break;
    case TRAIN_RUN_BACK:
        work->pos.z += VARS(work)->speed;
        if (work->pos.z <= BRAKE_Z(work)) {
            work->mode = TRAIN_BRAKE_BACK;
        }
        break;
    case TRAIN_BRAKE_BACK:
        speed = VARS(work)->speed + ACCEL;
        if (speed >= 0.0F) {
            VARS(work)->wait = WAIT_STEPS;
            work->mode = TRAIN_WAIT;
            speed = 0.0F;
            work->submode = TRAIN_SPEED_UP;
        }
        VARS(work)->speed = speed;
        cargo_train_speed = speed;
        work->pos.z += speed;
        break;
    }
    task_step_children(task);
}

static void cargo_train_destroy(struct task *task)
{
    task_destroy_children(task);
}
