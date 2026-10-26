/* O GACHA and O GSHOOTER machines that launch E GACHA robots */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/ground.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "effect/dust.h"
#include "object/placement.h"
#include "sound/sound.h"
#include "stg10/gacha_machine.h"
#include "stg10/gacha_robot.h"
#include "stg10/puff.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_ACTION gacha_machine_action;
extern NJS_ACTION gacha_shooter_action;

#define SE_GACHA_LAUNCH 282
#define SE_GACHA_SHOOT 26

/* the steps a machine waits, from its start and after each launch, before it
 * may launch */
#define WAIT 100.0F

/* a work flag: the machine's next draw shows a robot inside it, and clears
 * the flag */
#define ROBOT_SHOWN 0x200

/* gacha_machine_action's frames a step as the machine opens or shuts, and
 * its frame with the machine open */
#define OPEN_SPEED 1.5F
#define OPEN_FRAME 28.99F

/* the steps from O GACHA standing open to its launch, and to its shutting */
#define LAUNCH_STEP 20
#define SHUT_STEP 40
/* the steps O GSHOOTER counts down before each shot, and the number of
 * speeds its shots cycle through */
#define SHOT_STEPS 60
#define SHOT_SPEEDS 3

/* how far ahead of O GACHA and of O GSHOOTER their robots' floor is found */
#define GACHA_AHEAD 10.0F
#define SHOOTER_AHEAD 100.0F

enum gacha_machine_mode {
    MACHINE_INIT,
    MACHINE_WAIT,
    MACHINE_LAUNCH,
    MACHINE_OPEN,
    MACHINE_SHUT,
    MACHINE_NEXT,
    MACHINE_DONE
};

enum gacha_shooter_mode {
    SHOOTER_INIT,
    SHOOTER_WAIT,
    SHOOTER_COUNT,
    SHOOTER_NEXT,
    SHOOTER_DONE
};

struct gacha_machine_vars {
    Uint16 left; /* launches left, from scale x. a scale of 0 gives 65536 */
    Uint16 most; /* the most robots out at once, from scale y */
    Float wait; /* the steps waited since the start or the last launch's end */
    Float frame; /* the frame of gacha_machine_action the machine shows */
};

#define VARS(work) ((struct gacha_machine_vars *)&(work)->vars)
/* the height of the floor GACHA_AHEAD in front of O GACHA, where its robots
 * land */
#define FLOOR(work) ((work)->scl.z)
/* the index in gacha_shooter_speeds of O GSHOOTER's next shot's speed */
#define SHOT(work) ((work)->submode)

static int gacha_machine_count_robots(struct task *task);
static void gacha_machine_destroy(struct task *task);
static void gacha_machine_draw(struct task *task);
static void gacha_machine_draw_at(struct task_work *work);
static void gacha_machine_launch(struct task *task);
static void gacha_shooter_draw(struct task *task);
static void gacha_shooter_destroy(struct task *task);
static void gacha_shooter_draw_at(struct task_work *work);
static void gacha_shooter_launch(struct task *task);

/* the puffs both machines raise as they launch a robot */
static struct dust_params gacha_machine_puff = {
    2.0F,
    0.038F,
    0.19F,
    0.06F,
    0.005F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 1.0F, 1.0F, 1.0F},
};

/* the robots' speed for each of O GSHOOTER's shots, used in turn */
static Float gacha_shooter_speeds[SHOT_SPEEDS] = {5.0F, 7.5F, 10.0F};

static void gacha_machine_set_counts(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->left = work->scl.x;
    VARS(work)->most = work->scl.y;
}

/* count a launch off. while launches are left, restart the wait and return
 * TRUE */
static Bool gacha_machine_next(struct task *task, struct task_work *work)
{
    VARS(work)->left--;
    if (VARS(work)->left <= 0) {
        return FALSE;
    }
    VARS(work)->wait = 0.0F;
    return TRUE;
}

/* count a step of the wait until it passes WAIT. after that, return TRUE
 * while fewer robots than VARS(work)->most are out */
static Bool gacha_machine_wait(struct task *task, struct task_work *work)
{
    if (VARS(work)->wait > WAIT) {
        if (gacha_machine_count_robots(task) < VARS(work)->most) {
            return TRUE;
        }
    } else {
        VARS(work)->wait += 1.0F;
    }
    return FALSE;
}

/* the height of the floor below a point in front of the machine, ahead units
 * away */
static Float gacha_machine_ground(struct task_work *work, Float ahead)
{
    struct angles slope;
    NJS_POINT3 point;
    Angle ang;

    point.x = ahead;
    point.y = 0.0F;
    point.z = 0.0F;
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njCalcPoint(NULL, &point, &point);
    njPopMatrix(1);
    return ground_below(point.x, point.y, point.z, &slope);
}

/* count the robots the machine launched that still live, which are its
 * children */
static int gacha_machine_count_robots(struct task *task)
{
    int count = 0;
    struct task *child;

    child = task->child;
    while (child) {
        child = child->next;
        count++;
    }
    return count;
}

/* a machine whose launches ran out stays shut when it is placed again */
void gacha_machine_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case MACHINE_INIT:
        gacha_machine_set_counts(task);
        work->timer = 0;
        FLOOR(work) = gacha_machine_ground(work, GACHA_AHEAD);
        task->destructor = gacha_machine_destroy;
        task->draw = gacha_machine_draw;
        if (placement_used(task)) {
            work->mode = MACHINE_DONE;
        } else {
            work->mode = MACHINE_WAIT;
        }
        break;
    case MACHINE_WAIT:
        if (edit_mode) {
            break;
        }
        if (gacha_machine_wait(task, work)) {
            work->mode = MACHINE_OPEN;
            VARS(work)->frame = 0.0F;
        }
        break;
    case MACHINE_OPEN:
        VARS(work)->frame += OPEN_SPEED;
        if (VARS(work)->frame > OPEN_FRAME) {
            VARS(work)->frame = OPEN_FRAME;
            work->timer = 0;
            work->mode = MACHINE_LAUNCH;
        }
        work->flags |= ROBOT_SHOWN;
        break;
    case MACHINE_LAUNCH:
        work->timer++;
        if (TASK_TIMER(work) < LAUNCH_STEP) {
            work->flags |= ROBOT_SHOWN;
        } else if (TASK_TIMER(work) == LAUNCH_STEP) {
            gacha_machine_launch(task);
            sound_play(SE_GACHA_LAUNCH, 0, 0, 0);
        } else if (TASK_TIMER(work) == SHUT_STEP) {
            work->timer = 0;
            work->mode = MACHINE_SHUT;
        }
        break;
    case MACHINE_SHUT:
        VARS(work)->frame -= OPEN_SPEED;
        if (VARS(work)->frame < 0.0F) {
            VARS(work)->frame = 0.0F;
            work->mode = MACHINE_NEXT;
        }
        break;
    case MACHINE_NEXT:
        if (gacha_machine_next(task, work)) {
            work->mode = MACHINE_WAIT;
        } else {
            placement_set_used(task);
            work->mode = MACHINE_DONE;
        }
        break;
    case MACHINE_DONE:
        break;
    }
    task_step_children(task);
    gacha_machine_draw_at(work);
}

static void gacha_machine_destroy(struct task *task)
{
    task_destroy_children(task);
}

static void gacha_machine_draw(struct task *task)
{
    gacha_machine_draw_at(task->work);
}

static void gacha_machine_draw_at(struct task_work *work)
{
    Angle ang;

    if (frame_step == 0) {
        njPushMatrix(NULL);
        texture_set_stage();
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y + NJM_DEG_ANG(90), ang);
        njScale(NULL, 1.5F, 1.5F, 1.5F);
        draw_action(&gacha_machine_action, VARS(work)->frame);
        if (work->flags & ROBOT_SHOWN) {
            njSetTexture(&gacha_robot_texlist);
            njTranslate(NULL, 0.0F, -1.0F, 0.0F);
            draw_model(gacha_robot_object->model);
            work->flags &= ~ROBOT_SHOWN;
        }
        njPopMatrix(1);
    }
}

static void gacha_machine_launch(struct task *task)
{
    int i;
    Angle ang;
    Angle turn;
    struct gacha_launch launch;
    struct task_work *work = task->work;

    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, turn);
    launch.kind = LAUNCH_WITH_SPEED;
    launch.pos.x = 1.0F;
    launch.pos.y = 0.0F;
    launch.pos.z = 0.0F;
    launch.speed.x = 1.4F;
    launch.speed.y = 0.0F;
    launch.speed.z = 0.0F;
    launch.ang_y = work->ang.y;
    launch.ground = FLOOR(work);
    njCalcPoint(NULL, &launch.pos, &launch.pos);
    njCalcVector(NULL, &launch.speed, &launch.speed);
    gacha_robot_launch(task, &launch);
    ang = RANDOM_ANGLE();
    for (i = 0; i < 8; i++, ang += NJM_DEG_ANG(45)) {
        gacha_machine_puff.pos.x = 0.0F;
        gacha_machine_puff.pos.y = 0.0F;
        gacha_machine_puff.pos.z = 0.0F;
        gacha_machine_puff.speed.x = 1.0F;
        gacha_machine_puff.speed.y = njCos(ang);
        gacha_machine_puff.speed.z = njSin(ang);
        gacha_machine_puff.scale = 1.0F + (rand() * RAND_SCALE);
        njCalcPoint(NULL, &gacha_machine_puff.pos, &gacha_machine_puff.pos);
        njCalcVector(NULL, &gacha_machine_puff.speed,
                     &gacha_machine_puff.speed);
        final_puff_create(&gacha_machine_puff);
    }
    njPopMatrix(1);
}

void gacha_shooter_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case SHOOTER_INIT:
        gacha_machine_set_counts(task);
        work->timer = 0;
        task->destructor = gacha_shooter_destroy;
        task->draw = gacha_shooter_draw;
        if (placement_used(task)) {
            work->mode = SHOOTER_DONE;
        } else {
            work->mode = SHOOTER_WAIT;
        }
        break;
    case SHOOTER_WAIT:
        if (edit_mode) {
            break;
        }
        if (gacha_machine_wait(task, work)) {
            work->mode = SHOOTER_COUNT;
            work->timer = SHOT_STEPS;
            VARS(work)->frame = 0.0F;
        }
        break;
    case SHOOTER_COUNT:
        if (work->timer == 0) {
            gacha_shooter_launch(task);
            sound_play(SE_GACHA_SHOOT, 0, 0, 0);
            SHOT(work)++;
            if (SHOT(work) == SHOT_SPEEDS) {
                SHOT(work) = 0;
            }
            work->mode = SHOOTER_NEXT;
        } else {
            work->timer--;
        }
        break;
    case SHOOTER_NEXT:
        if (gacha_machine_next(task, work)) {
            work->mode = SHOOTER_WAIT;
        } else {
            placement_set_used(task);
            work->mode = SHOOTER_DONE;
        }
        break;
    case SHOOTER_DONE:
        break;
    }
    task_step_children(task);
    gacha_shooter_draw_at(work);
}

static void gacha_shooter_draw(struct task *task)
{
    gacha_shooter_draw_at(task->work);
}

static void gacha_shooter_destroy(struct task *task)
{
    task_destroy_children(task);
}

/* draw the shooter as gacha_shooter_action's object stands, unanimated */
static void gacha_shooter_draw_at(struct task_work *work)
{
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y + NJM_DEG_ANG(90), ang);
        draw_tree(gacha_shooter_action.object);
        njPopMatrix(1);
    }
}

static void gacha_shooter_launch(struct task *task)
{
    int i;
    Angle ang;
    Angle turn;
    struct gacha_launch launch;
    struct task_work *work = task->work;

    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, turn);
    launch.kind = LAUNCH_WITH_SPEED;
    launch.pos.x = 10.0F;
    launch.pos.y = 0.0F;
    launch.pos.z = 0.0F;
    launch.speed.x = gacha_shooter_speeds[SHOT(work)];
    launch.speed.y = 0.0F;
    launch.speed.z = 0.0F;
    launch.ang_y = work->ang.y;
    launch.ground = gacha_machine_ground(work, SHOOTER_AHEAD);
    njCalcPoint(NULL, &launch.pos, &launch.pos);
    njCalcVector(NULL, &launch.speed, &launch.speed);
    gacha_robot_launch(task, &launch);
    ang = RANDOM_ANGLE();
    for (i = 0; i < 8; i++, ang += NJM_DEG_ANG(45)) {
        gacha_machine_puff.pos.x = 30.0F;
        gacha_machine_puff.pos.y = 0.0F;
        gacha_machine_puff.pos.z = 0.0F;
        gacha_machine_puff.speed.x = 1.0F;
        gacha_machine_puff.speed.y = njCos(ang);
        gacha_machine_puff.speed.z = njSin(ang);
        gacha_machine_puff.scale = (1.0F + (rand() * RAND_SCALE)) * 2.0F;
        njCalcPoint(NULL, &gacha_machine_puff.pos, &gacha_machine_puff.pos);
        njCalcVector(NULL, &gacha_machine_puff.speed,
                     &gacha_machine_puff.speed);
        final_puff_create(&gacha_machine_puff);
    }
    ang += NJM_DEG_ANG(22.5);
    for (i = 0; i < 8; i++, ang += NJM_DEG_ANG(45)) {
        gacha_machine_puff.pos.x = 30.0F;
        gacha_machine_puff.pos.y = 0.0F;
        gacha_machine_puff.pos.z = 0.0F;
        gacha_machine_puff.speed.x = 1.0F;
        gacha_machine_puff.speed.y = njCos(ang) * 1.5F;
        gacha_machine_puff.speed.z = njSin(ang) * 1.5F;
        gacha_machine_puff.scale = (1.0F + (rand() * RAND_SCALE)) * 3.0F;
        njCalcPoint(NULL, &gacha_machine_puff.pos, &gacha_machine_puff.pos);
        njCalcVector(NULL, &gacha_machine_puff.speed,
                     &gacha_machine_puff.speed);
        final_puff_create(&gacha_machine_puff);
    }
    njPopMatrix(1);
}
