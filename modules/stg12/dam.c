/* act 0's dam, a wall that sinks once switch 0 turns on. the water it holds
 * back then spills into the water below */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/tree.h"
#include "object/switch.h"
#include "player/big.h"
#include "stage/stage.h"
#include "stg12/dam.h"
#include "stg12/dam_spill.h"
#include "stg12/dam_water.h"
#include "stg12/water.h"
#include "system/task.h"
#include "system/texture.h"

extern NJS_OBJECT shelter_dam_object;

/* the heights of the wall's top and of the water it holds back, as the dam
 * starts */
#define WALL_HEIGHT 75.0F
#define WATER_HEIGHT 65.0F

/* how far the wall sinks, and later rises, each step */
#define SINK_SPEED 0.15F
#define RISE_SPEED 0.6F

/* the water below, shelter_water_level, rises FILL_SPEED a step while the spill
 * pours into it. FILL_SPEED is 69.9/65 of SINK_SPEED. at that rate, it rises
 * from -80 to FILL_LEVEL as the dam's water falls from 65 to 0. once the wall
 * has sunk, the water below rises on to FILL_LEVEL */
#define FILL_SPEED (69.9F / WATER_HEIGHT * SINK_SPEED)
#define FILL_LEVEL (-10.1F)

/* the steps the dam waits once the water below has risen, before it ends
 * the spill and rises */
#define RISEN_STEPS 60

/* the dam's modes, in order. DAM_WAIT waits for DAM_SWITCH. DAM_SINK sinks
 * the wall until its top is down to its water, and starts the spill.
 * DAM_SPILL sinks it on until its top is down to 0, and its water falls with
 * the top. DAM_SUNK raises the water below on to FILL_LEVEL. DAM_RISEN waits
 * RISEN_STEPS, then ends the spill and its water's surface. DAM_RISE raises
 * the wall back, and DAM_UP leaves it there */
enum shelter_dam_mode {
    DAM_WAIT,
    DAM_SINK,
    DAM_SPILL,
    DAM_SUNK,
    DAM_RISEN,
    DAM_RISE,
    DAM_UP
};

/* TASK_TIMER(work) holds the stage and act the dam was made for. the step
 * kills the task once they change */
struct shelter_dam_vars {
    Uint8 unk_00[4];
    Float top; /* height of the wall's top */
};

#define VARS(work) ((struct shelter_dam_vars *)&(work)->vars)

int shelter_dam_flags;
Float shelter_dam_level;

static void shelter_dam_step(struct task *task);
static void shelter_dam_draw(struct task *task);
static void shelter_dam_destroy(struct task *task);

void shelter_dam_start(struct task *task)
{
    struct task_work *work = task->work;

    work->mode = DAM_WAIT;
    TASK_TIMER(work) = stage_get_stage_act();
    work->pos.x = DAM_X;
    work->pos.y = 0.0F;
    work->pos.z = DAM_Z;
    VARS(work)->top = WALL_HEIGHT;
    shelter_dam_level = WATER_HEIGHT;
    switch_set(DAM_SWITCH, FALSE);
    shelter_dam_flags = 0;
    task_create_child(TASK_WORK, shelter_dam_water_start, task);
    task->step = shelter_dam_step;
    task->draw = shelter_dam_draw;
    task->destructor = shelter_dam_destroy;
}

static void shelter_dam_step(struct task *task)
{
    struct task_work *work = task->work;
    Float top;

    if (TASK_TIMER(work) != stage_get_stage_act()) {
        task_kill(task);
        return;
    }
    switch (work->mode) {
    case DAM_WAIT:
        if (switch_is_on(DAM_SWITCH)) {
            work->mode = DAM_SINK;
            shelter_water_state = SHELTER_WATER_RISING;
        }
        shelter_dam_draw(task);
        task_step_children(task);
        break;
    case DAM_SINK:
        top = VARS(work)->top - SINK_SPEED;
        if (top <= WATER_HEIGHT) {
            task_create_child(TASK_WORK, shelter_dam_spill_start, task);
            shelter_dam_level = top;
            work->mode = DAM_SPILL;
        }
        work->pos.y -= SINK_SPEED;
        VARS(work)->top = top;
        shelter_dam_draw(task);
        task_step_children(task);
        break;
    case DAM_SPILL:
        top = VARS(work)->top - SINK_SPEED;
        if (top <= 0.0F) {
            work->mode = DAM_SUNK;
        }
        work->pos.y -= SINK_SPEED;
        VARS(work)->top = top;
        shelter_dam_level = top;
        if (shelter_dam_flags & DAM_SPILLING) {
            shelter_water_level += FILL_SPEED;
        }
        shelter_dam_draw(task);
        task_step_children(task);
        break;
    case DAM_SUNK:
        shelter_water_level += FILL_SPEED;
        if (shelter_water_level >= FILL_LEVEL) {
            work->tick = RISEN_STEPS;
            work->mode = DAM_RISEN;
            shelter_water_state = SHELTER_WATER_RISEN;
            fishing_shelter_ready = TRUE; /* Hot Shelter's fish may move */
        }
        task_step_children(task);
        break;
    case DAM_RISEN:
        work->tick--;
        if (work->tick <= 0) {
            task_destroy_children(task);
            work->mode = DAM_RISE;
        } else {
            task_step_children(task);
        }
        break;
    case DAM_RISE:
        work->pos.y += RISE_SPEED;
        if (work->pos.y >= 0.0F) {
            work->pos.y = 0.0F;
            work->mode = DAM_UP;
        }
        /* falls through, so the wall is drawn as it rises */
    case DAM_UP:
        shelter_dam_draw(task);
        break;
    }
}

static void shelter_dam_draw(struct task *task)
{
    struct task_work *work = task->work;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    draw_tree(&shelter_dam_object);
    njPopMatrix(1);
}

static void shelter_dam_destroy(struct task *task)
{
    task_destroy_children(task);
}
