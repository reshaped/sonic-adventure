/* texture animations of the land in the sewer, station and hotel acts */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/land_anim.h"
#include "stage/stage.h"
#include "system/task.h"

/* land parts whose textures animate */
extern NJS_OBJECT dat_0c9c3868;
extern NJS_OBJECT dat_0c9da7f8;
extern NJS_OBJECT dat_0ca15a18;
extern NJS_OBJECT dat_0ca34a58;
extern NJS_OBJECT dat_0c9e8870;
extern NJS_OBJECT dat_0ca241b4;
extern NJS_OBJECT dat_0ca232ac;
extern NJS_OBJECT dat_0ca246b4;
extern NJS_OBJECT dat_0ca4c3e8;
extern NJS_OBJECT dat_0ca4ce64;
extern NJS_OBJECT dat_0ca528a0;
extern NJS_OBJECT dat_0ca5716c;
extern NJS_OBJECT dat_0ca48fd0;
extern NJS_OBJECT dat_0ca5279c;
extern NJS_OBJECT dat_0ca49518;

struct land_anim_part {
    NJS_OBJECT *object;
    Uint8 count; /* how many materials animate */
    Uint8 materials[3]; /* indexes of the model's animated materials */
};

struct land_anim {
    struct land_anim_part *parts;
    Uint8 count; /* how many parts animate */
    Uint8 frames;
    Uint16 first; /* the texture frame 0 shows */
    Uint16 rest; /* each later frame n shows texture rest + n */
    Uint16 delay; /* calls of land_anim_advance each frame lasts */
};

/* work->tick holds the frame shown, and work->timer the act the task started
 * in */
struct land_anim_vars {
    struct land_anim *anim;
    Sint32 shown; /* calls the current frame has lasted */
};

/* ANIM_HOLD stops the step advancing the animation. only this file reaches the
 * task, and it never sets that mode */
enum land_anim_mode { ANIM_START, ANIM_RUN, ANIM_HOLD };

#define VARS(work) ((struct land_anim_vars *)&(work)->vars)

static void station_square_land_anim_step(struct task *task);
static void land_anim_advance(struct task *task);
static void station_square_land_anim_destroy(struct task *task);

/* the sewer act's animation */
static struct land_anim_part gesui_parts[] = {
    {&dat_0c9c3868, 1, {0}},
    {&dat_0c9da7f8, 1, {0}},
};
static struct land_anim gesui_anim = {
#include "adv00/land_anim/gesui_anim.inc"
};

/* the station act's three */
static struct land_anim_part station_a_parts[] = {
    {&dat_0ca15a18, 1, {0}},
    {&dat_0ca34a58, 1, {0}},
};
static struct land_anim station_a_anim = {
#include "adv00/land_anim/station_a_anim.inc"
};

static struct land_anim_part station_b_parts[] = {
    {&dat_0c9e8870, 1, {2}},
    {&dat_0ca241b4, 1, {3}},
};
static struct land_anim station_b_anim = {
#include "adv00/land_anim/station_b_anim.inc"
};

static struct land_anim_part station_c_parts[] = {
    {&dat_0ca232ac, 1, {0}},
    {&dat_0ca246b4, 1, {0}},
};
static struct land_anim station_c_anim = {
#include "adv00/land_anim/station_c_anim.inc"
};

/* the hotel act's three */
static struct land_anim_part hotel_a_parts[] = {
    {&dat_0ca4c3e8, 1, {0}},
    {&dat_0ca4ce64, 1, {0}},
    {&dat_0ca528a0, 1, {0}},
    {&dat_0ca5716c, 1, {0}},
};
static struct land_anim hotel_a_anim = {
#include "adv00/land_anim/hotel_a_anim.inc"
};

static struct land_anim_part hotel_b_parts[] = {
    {&dat_0ca48fd0, 1, {2}},
    {&dat_0ca5279c, 1, {1}},
};
static struct land_anim hotel_b_anim = {
#include "adv00/land_anim/hotel_b_anim.inc"
};

static struct land_anim_part hotel_c_parts[] = {
    {&dat_0ca49518, 1, {0}},
    {&dat_0ca48fd0, 1, {0}},
    {&dat_0ca5279c, 1, {2}},
};
static struct land_anim hotel_c_anim = {
#include "adv00/land_anim/hotel_c_anim.inc"
};

static struct land_anim *land_anims[] = {
    &gesui_anim,   &station_a_anim, &station_b_anim, &station_c_anim,
    &hotel_a_anim, &hotel_b_anim,   &hotel_c_anim,
};

void land_anim_start(enum land_anim_index anim)
{
    struct task *task =
        task_create(TASK_WORK, 3, station_square_land_anim_step);
    struct task_work *work = task->work;

    work->mode = ANIM_START;
    VARS(work)->anim = land_anims[anim];
    work->timer = stage_get_stage_act();
}

/* advance the animation each step, and end the task once the act
 * changes */
static void station_square_land_anim_step(struct task *task)
{
    struct task_work *work = task->work;
    int act = stage_get_stage_act();

    if (act != TASK_TIMER(work)) {
        task_kill(task);
        return;
    }
    switch (work->mode) {
    case ANIM_START:
        task->destructor = station_square_land_anim_destroy;
        task->draw = land_anim_advance;
        work->mode = ANIM_RUN;
        work->tick = 0;
        VARS(work)->shown = 0;
        break;
    case ANIM_RUN:
        land_anim_advance(task);
        break;
    case ANIM_HOLD:
        break;
    default:
        work->mode = ANIM_START;
        break;
    }
}

/* count the call toward the frame's delay. once the frame has lasted delay
 * calls, move to the next frame, or from the last back to 0. then set each
 * part's materials to the frame's texture. the step calls it, and it is the
 * task's draw too. draws run while the game is paused and steps don't, so the
 * textures animate in a pause */
static void land_anim_advance(struct task *task)
{
    struct task_work *work = task->work;
    struct land_anim *anim = VARS(work)->anim;
    int frames = anim->frames;
    int count = anim->count;
    int delay = anim->delay;
    Uint8 frame;
    int texture;
    int i;
    int j;

    VARS(work)->shown++;
    frame = work->tick;
    if (VARS(work)->shown == delay) {
        frame++;
        VARS(work)->shown = 0;
        if (frame >= frames) {
            frame = 0;
        }
    }
    if (frame == 0) {
        texture = anim->first;
    } else {
        texture = anim->rest + frame;
    }
    work->tick = frame;
    for (i = 0; i < count; i++) {
        struct land_anim_part *part = &anim->parts[i];
        Uint8 materials = part->count;
        NJS_OBJECT *object = part->object;

        for (j = 0; j < materials; j++) {
            object->model->mats[part->materials[j]].attr_texId = texture;
        }
    }
}

static void station_square_land_anim_destroy(struct task *task)
{
}
