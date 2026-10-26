/* the ruin's water textures: a task for each act, which cycles the textures
 * of the act's water. act 2's task also draws its water */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/model.h"
#include "stage/stage.h"
#include "stg07/obj_ruin_textures.h"
#include "stg07/textures.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT water_surface_object;
extern NJS_OBJECT ruin_act0_water_1_object;
extern NJS_OBJECT ruin_act0_water_2_object;
extern NJS_OBJECT ruin_act0_water_3_object;
extern NJS_OBJECT ruin_act1_water_1_object;
extern NJS_OBJECT ruin_act1_water_2_object;
extern NJS_OBJECT ruin_act1_water_3_object;
extern NJS_OBJECT ruin_act1_water_4_object;
extern NJS_OBJECT ruin_act1_water_5_object;
extern NJS_OBJECT ruin_act1_water_6_object;
extern NJS_OBJECT ruin_act1_water_7_object;
extern NJS_OBJECT ruin_act1_water_8_object;
extern NJS_OBJECT ruin_act1_water_9_object;
extern NJS_OBJECT ruin_act2_water_object;

/* a model whose materials show the water's frames */
struct ruin_water_part {
    NJS_OBJECT *object;
    Uint8 count;
    Uint8 materials[3]; /* indexes of the model's animated materials */
    Uint8 first; /* the texture of the first frame */
    /* the calls of ruin_textures_advance each water texture shows for. the
     * advance reads only the first part's delay */
    Sint16 delay;
};

/* acts 0 and 1: work->tick is the texture shown, and work->timer the calls
 * it has lasted */
struct ruin_textures_vars {
    struct ruin_water_part *parts;
    Sint8 count;
    Sint8 frames;
};

/* act 2: work->tick is the frame shown */
struct ruin_water_vars {
    NJS_OBJECT **objects;
    Sint8 count;
    Sint8 frames;
};

#define TEXTURES(work) ((struct ruin_textures_vars *)&(work)->vars)
#define WATER(work) ((struct ruin_water_vars *)&(work)->vars)

enum textures_mode { TEXTURES_START, TEXTURES_RUN, TEXTURES_HOLD };

/* the water has WATER_FRAMES frames. in act 0's and act 1's texlists, they
 * start at ACT0_WATER_FIRST and ACT1_WATER_FIRST. in ruin_water_texlist, which
 * act 2's water draws with, they start at 0 */
#define WATER_FRAMES 14
#define ACT0_WATER_FIRST 44
#define ACT1_WATER_FIRST 81

/* the models each act's water shows its frames on */
#define ACT0_PARTS 4
#define ACT1_PARTS 9
#define ACT2_OBJECTS 1

static struct ruin_water_part ruin_act0_water[ACT0_PARTS] = {
#include "stg07/textures/ruin_act0_water.inc"
};
static struct ruin_water_part ruin_act1_water[ACT1_PARTS] = {
#include "stg07/textures/ruin_act1_water.inc"
};
static NJS_OBJECT *ruin_act2_water[ACT2_OBJECTS] = {&ruin_act2_water_object};

static void ruin_textures_advance(struct task *task);
static void ruin_act2_water_draw(struct task *task);
static void ruin_textures_destroy(struct task *task);

/* the step of act 0's task. in any other act, it ends the task */
void ruin_act0_textures_step(struct task *task)
{
    struct task_work *work = task->work;
    int act = stage_get_stage_act();

    if (act != STAGE_ACT(STAGE_RUIN, 0)) {
        task_kill(task);
        return;
    }
    switch (work->mode) {
    case TEXTURES_START:
        task->destructor = ruin_textures_destroy;
        task->draw = ruin_textures_advance;
        work->mode = TEXTURES_RUN;
        work->tick = ACT0_WATER_FIRST;
        TEXTURES(work)->parts = ruin_act0_water;
        TEXTURES(work)->count = ACT0_PARTS;
        TEXTURES(work)->frames = WATER_FRAMES;
        break;
    case TEXTURES_RUN:
        ruin_textures_advance(task);
        /* @hack spelling: a return where the function returns anyway, which
         * makes the call a tail branch, BRA to ruin_textures_advance
         * (0x0c90a6f8). with a break, 2 of ruin_act0_textures_step's 52 words
         * differ */
        return;
    case TEXTURES_HOLD:
        break;
    default:
        work->mode = TEXTURES_START;
        break;
    }
}

void ruin_act1_textures_step(struct task *task)
{
    struct task_work *work = task->work;
    int act = stage_get_stage_act();

    if (act != STAGE_ACT(STAGE_RUIN, 1)) {
        task_kill(task);
        return;
    }
    switch (work->mode) {
    case TEXTURES_START:
        task->destructor = ruin_textures_destroy;
        task->draw = ruin_textures_advance;
        work->mode = TEXTURES_RUN;
        work->tick = ACT1_WATER_FIRST;
        TEXTURES(work)->parts = ruin_act1_water;
        TEXTURES(work)->count = ACT1_PARTS;
        TEXTURES(work)->frames = WATER_FRAMES;
        break;
    case TEXTURES_RUN:
        ruin_textures_advance(task);
        /* @hack spelling: a return where the function returns anyway, which
         * makes the call a tail branch, BRA to ruin_textures_advance
         * (0x0c90a7a6). with a break, 2 of ruin_act1_textures_step's 52 words
         * differ */
        return;
    case TEXTURES_HOLD:
        break;
    default:
        work->mode = TEXTURES_START;
        break;
    }
}

/* count a call, and advance a frame once the first part's delay has
 * passed. then show the frame on every part. the step and the draw both call
 * this. the draw runs while the game is paused, so the water moves on during
 * a pause */
static void ruin_textures_advance(struct task *task)
{
    struct task_work *work;
    /* @hack spelling: work assigned inside parts's initializer, which loads
     * work into R5 and parts into R13 (0x0c90a7bc, 0x0c90a7c0). with work set
     * in its own declaration, 23 of ruin_textures_advance's 74 words differ */
    struct ruin_water_part *parts = TEXTURES(work = task->work)->parts;
    struct ruin_water_part *part;
    NJS_OBJECT *object;
    int i;
    int j;

    work->timer++;
    if (work->timer == parts->delay) {
        work->tick++;
        if (work->tick >= TEXTURES(work)->frames + parts->first) {
            work->tick = parts->first;
        }
        work->timer = 0;
    }
    for (i = 0; i < TEXTURES(work)->count; i++) {
        part = &parts[i];
        object = part->object;
        for (j = 0; j < part->count; j++) {
            object->model->mats[part->materials[j]].attr_texId = work->tick;
        }
    }
}

void ruin_act2_textures_step(struct task *task)
{
    struct task_work *work = task->work;
    int act = stage_get_stage_act();

    if (act != STAGE_ACT(STAGE_RUIN, 2)) {
        task_kill(task);
        return;
    }
    switch (work->mode) {
    case TEXTURES_START:
        task->destructor = ruin_textures_destroy;
        task->draw = ruin_act2_water_draw;
        work->tick = 0;
        work->mode = TEXTURES_RUN;
        WATER(work)->objects = ruin_act2_water;
        WATER(work)->count = ACT2_OBJECTS;
        WATER(work)->frames = WATER_FRAMES;
        break;
    case TEXTURES_RUN:
        ruin_act2_water_draw(task);
        break;
    case TEXTURES_HOLD:
        break;
    default:
        work->mode = TEXTURES_START;
        break;
    }
}

/* show the next frame and draw the water. the frame wraps to 0 once it reaches
 * frames - 1. the last of the frames never shows. the water rises by the sine
 * of work->timer in degrees, but nothing advances the timer. the step and the
 * draw both call this */
static void ruin_act2_water_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;
    int i;

    work->tick++;
    njSetTexture(&ruin_water_texlist);
    if (work->tick >= WATER(work)->frames - 1) {
        work->tick = 0;
    }
    for (i = 0; i < WATER(work)->count; i++) {
        /* @hack spelling: the list through a local, which SHC reads into R4
         * each pass and indexes by MOV.L @(R0,R4),R13 (0x0c90a930,
         * 0x0c90a934). indexed in place, 4 of ruin_act2_water_draw's 83 words
         * differ */
        NJS_OBJECT **objects = WATER(work)->objects;

        object = objects[i];
        object->model->mats[0].attr_texId = work->tick;
        njPushMatrix(NULL);
        njTranslate(NULL, object->pos[0],
                    object->pos[1] +
                        njSin((Angle)(TASK_TIMER(work) * ANGLE_PER_DEGREE)),
                    object->pos[2]);
        draw_model(object->model);
        njPopMatrix(1);
    }
}

static void ruin_textures_destroy(struct task *task)
{
}
