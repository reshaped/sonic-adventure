/* Emerald Coast's texture cycles. each steps the materials of an act's
 * sand, waves or water through a run of textures */
#include <ninja.h>
#include <sg_xpt.h>

#include "stage/stage.h"
#include "stg01/cycle.h"
#include "system/task.h"

extern NJS_OBJECT beach_act0_sand_object_0;
extern NJS_OBJECT beach_act0_sand_object_1;
extern NJS_OBJECT beach_act0_sand_object_2;
extern NJS_OBJECT beach_act0_sand_object_3;
extern NJS_OBJECT beach_act0_sand_object_4;
extern NJS_OBJECT beach_act0_sand_object_5;
extern NJS_OBJECT beach_act0_sand_object_6;
extern NJS_OBJECT beach_act0_sand_object_7;
extern NJS_OBJECT beach_act0_sand_object_8;
extern NJS_OBJECT beach_act0_sand_object_9;
extern NJS_OBJECT beach_act0_sand_object_10;
extern NJS_OBJECT beach_act0_sand_object_11;
extern NJS_OBJECT beach_act0_wave_object_0;
extern NJS_OBJECT beach_act0_wave_object_1;
extern NJS_OBJECT beach_act0_wave_object_2;
extern NJS_OBJECT beach_act0_wave_object_3;
extern NJS_OBJECT beach_act0_wave_object_4;
extern NJS_OBJECT beach_act0_wave_object_5;
extern NJS_OBJECT beach_act0_wave_object_6;
extern NJS_OBJECT beach_act0_wave_object_7;
extern NJS_OBJECT beach_act0_wave_object_8;
extern NJS_OBJECT beach_act0_wave_object_9;
extern NJS_OBJECT beach_act0_wave_object_10;
extern NJS_OBJECT beach_act0_wave_object_11;
extern NJS_OBJECT beach_act0_wave_object_12;
extern NJS_OBJECT beach_act0_wave_object_13;
extern NJS_OBJECT beach_act0_wave_object_14;
extern NJS_OBJECT beach_act0_wave_object_15;
extern NJS_OBJECT beach_act0_wave_object_16;
extern NJS_OBJECT beach_act1_sand_object_0;
extern NJS_OBJECT beach_act1_sand_object_1;
extern NJS_OBJECT beach_act1_sand_object_2;
extern NJS_OBJECT beach_act1_sand_object_3;
extern NJS_OBJECT beach_act1_sand_object_4;
extern NJS_OBJECT beach_act1_sand_object_5;
extern NJS_OBJECT beach_act1_sand_object_6;
extern NJS_OBJECT beach_act1_sand_object_7;
extern NJS_OBJECT beach_act1_sand_object_8;
extern NJS_OBJECT beach_act1_sand_object_9;
extern NJS_OBJECT beach_act1_sand_object_10;
extern NJS_OBJECT beach_act1_sand_object_11;
extern NJS_OBJECT beach_act1_wave_object_0;
extern NJS_OBJECT beach_act1_wave_object_1;
extern NJS_OBJECT beach_act1_wave_object_2;
extern NJS_OBJECT beach_act1_wave_object_3;
extern NJS_OBJECT beach_act1_wave_object_4;
extern NJS_OBJECT beach_act1_wave_object_5;
extern NJS_OBJECT beach_act1_wave_object_6;
extern NJS_OBJECT beach_act1_wave_object_7;
extern NJS_OBJECT beach_act1_wave_object_8;
extern NJS_OBJECT beach_act1_wave_object_9;
extern NJS_OBJECT beach_act1_wave_object_10;
extern NJS_OBJECT beach_act1_wave_object_11;
extern NJS_OBJECT beach_act1_wave_object_12;
extern NJS_OBJECT beach_act1_wave_object_13;
extern NJS_OBJECT beach_act1_water_object_0;
extern NJS_OBJECT beach_act2_sand_object_0;
extern NJS_OBJECT beach_act2_sand_object_1;
extern NJS_OBJECT beach_act2_sand_object_2;
extern NJS_OBJECT beach_act2_sand_object_3;
extern NJS_OBJECT beach_act2_sand_object_4;
extern NJS_OBJECT beach_act2_sand_object_5;
extern NJS_OBJECT beach_act2_sand_object_6;
extern NJS_OBJECT beach_act2_sand_object_7;
extern NJS_OBJECT beach_act2_sand_object_8;
extern NJS_OBJECT beach_act2_sand_object_9;
extern NJS_OBJECT beach_act2_sand_object_10;
extern NJS_OBJECT beach_act2_sand_object_11;
extern NJS_OBJECT beach_act2_sand_object_12;
extern NJS_OBJECT beach_act2_sand_object_13;
extern NJS_OBJECT beach_act2_sand_object_14;
extern NJS_OBJECT beach_act2_sand_object_15;
extern NJS_OBJECT beach_act2_sand_object_16;
extern NJS_OBJECT beach_act2_sand_object_17;
extern NJS_OBJECT beach_act2_sand_object_18;
extern NJS_OBJECT beach_act2_sand_object_19;
extern NJS_OBJECT beach_act2_sand_object_20;
extern NJS_OBJECT beach_act2_sand_object_21;
extern NJS_OBJECT beach_act2_sand_object_22;
extern NJS_OBJECT beach_act2_sand_object_23;
extern NJS_OBJECT beach_act2_sand_object_24;
extern NJS_OBJECT beach_act2_sand_object_25;
extern NJS_OBJECT beach_act2_sand_object_26;
extern NJS_OBJECT beach_act2_sand_object_27;
extern NJS_OBJECT beach_act2_sand_object_28;
extern NJS_OBJECT beach_act2_wave_object_0;
extern NJS_OBJECT beach_act2_wave_object_1;
extern NJS_OBJECT beach_act2_wave_object_2;
extern NJS_OBJECT beach_act2_wave_object_3;
extern NJS_OBJECT beach_act2_wave_object_4;
extern NJS_OBJECT beach_act2_wave_object_5;
extern NJS_OBJECT beach_act2_wave_object_6;
extern NJS_OBJECT beach_act2_wave_object_7;
extern NJS_OBJECT beach_act2_wave_object_8;
extern NJS_OBJECT beach_act2_water_object_0;

/* each cycle's first texture, by its index in its act's land texlist:
 * m128_sand001, m128_wave001 or los_wt00 */
#define ACT0_SAND 67
#define ACT0_WAVE 82
#define ACT1_SAND 42
#define ACT1_WATER 57
#define ACT1_WAVE 71
#define ACT2_WAVE 50
#define ACT2_SAND 65
#define ACT2_WATER 80

/* the textures in each cycle: m128_sand001 to m128_sand015, m128_wave001 to
 * m128_wave015, and los_wt00 to los_wt11 */
#define SAND_TEXTURES 15
#define WAVE_TEXTURES 15
#define WATER_TEXTURES 12

/* a model whose materials take the cycle's texture. beach_cycle_advance
 * reads first and delay from the first entry alone */
struct cycle_entry {
    NJS_OBJECT *object;
    Uint8 count; /* how many of materials are in use */
    Uint8 materials[3]; /* indices into the model's mats */
    Uint8 first; /* the cycle's first texture */
    Uint8 unk_09;
    Sint16 delay; /* the calls of beach_cycle_advance each texture lasts */
};

/* the cycle's work->vars. work->tick holds the texture shown, and
 * work->timer the calls of beach_cycle_advance since it changed. the wave
 * tasks also set work->scl.z to 1, which no code here reads */
struct cycle_vars {
    struct cycle_entry *entries;
    Sint8 count;
    Sint8 textures; /* the cycle's length */
};

#define VARS(work) ((struct cycle_vars *)&(work)->vars)

/* the count of a cycle table's entries */
#define ENTRIES(cycles) (sizeof(cycles) / sizeof((cycles)[0]))

enum cycle_mode { CYCLE_INIT, CYCLE_RUN, CYCLE_STILL };

static void beach_cycle_advance(struct task *task);
static void beach_cycle_destroy(struct task *task);

static struct cycle_entry beach_act0_sand_cycles[12] = {
#include "stg01/cycle/beach_act0_sand_cycles.inc"
};

static struct cycle_entry beach_act0_wave_cycles[17] = {
#include "stg01/cycle/beach_act0_wave_cycles.inc"
};

static struct cycle_entry beach_act1_sand_cycles[12] = {
#include "stg01/cycle/beach_act1_sand_cycles.inc"
};

static struct cycle_entry beach_act1_wave_cycles[14] = {
#include "stg01/cycle/beach_act1_wave_cycles.inc"
};

static struct cycle_entry beach_act1_water_cycles[1] = {
#include "stg01/cycle/beach_act1_water_cycles.inc"
};

/* the last entry has a null object and a count of 0. ENTRIES counts it
 * with the rest, and its count keeps beach_cycle_advance from reading
 * through the null */
static struct cycle_entry beach_act2_sand_cycles[30] = {
#include "stg01/cycle/beach_act2_sand_cycles.inc"
};

static struct cycle_entry beach_act2_wave_cycles[9] = {
#include "stg01/cycle/beach_act2_wave_cycles.inc"
};

static struct cycle_entry beach_act2_water_cycles[1] = {
#include "stg01/cycle/beach_act2_water_cycles.inc"
};

void beach_act0_sand_step(struct task *task)
{
    struct task_work *work = task->work;
    int stage_act = stage_get_stage_act();

    if (stage_act != STAGE_ACT(STAGE_BEACH, 0)) {
        task_kill(task);
        return;
    }
    switch (work->mode) {
    case CYCLE_INIT:
        task->destructor = beach_cycle_destroy;
        task->draw = beach_cycle_advance;
        work->mode = CYCLE_RUN;
        work->tick = ACT0_SAND;
        VARS(work)->entries = beach_act0_sand_cycles;
        VARS(work)->count = ENTRIES(beach_act0_sand_cycles);
        VARS(work)->textures = SAND_TEXTURES;
        break;
    case CYCLE_RUN:
        beach_cycle_advance(task);
        return;
    case CYCLE_STILL:
        break;
    default:
        work->mode = CYCLE_INIT;
    }
}

void beach_act0_wave_step(struct task *task)
{
    struct task_work *work = task->work;
    int stage_act = stage_get_stage_act();

    if (stage_act != STAGE_ACT(STAGE_BEACH, 0)) {
        task_kill(task);
        return;
    }
    switch (work->mode) {
    case CYCLE_INIT:
        task->destructor = beach_cycle_destroy;
        task->draw = beach_cycle_advance;
        work->mode = CYCLE_RUN;
        work->tick = ACT0_WAVE;
        VARS(work)->entries = beach_act0_wave_cycles;
        VARS(work)->count = ENTRIES(beach_act0_wave_cycles);
        VARS(work)->textures = WAVE_TEXTURES;
        work->scl.z = 1.0F;
        break;
    case CYCLE_RUN:
        beach_cycle_advance(task);
        return;
    case CYCLE_STILL:
        break;
    default:
        work->mode = CYCLE_INIT;
    }
}

void beach_act1_wave_step(struct task *task)
{
    struct task_work *work = task->work;
    int stage_act = stage_get_stage_act();

    if (stage_act != STAGE_ACT(STAGE_BEACH, 1)) {
        task_kill(task);
        return;
    }
    switch (work->mode) {
    case CYCLE_INIT:
        task->destructor = beach_cycle_destroy;
        task->draw = beach_cycle_advance;
        work->mode = CYCLE_RUN;
        /* los_wt04, the water's fifth texture. past the water cycle's twelve,
         * act 1's texlist holds los_wt12 and los_wt13, at 69 and 70. the tick
         * counts up through them into the waves' own textures, and cycles
         * there */
        work->tick = ACT1_WATER + 4;
        VARS(work)->entries = beach_act1_wave_cycles;
        VARS(work)->count = ENTRIES(beach_act1_wave_cycles);
        VARS(work)->textures = WAVE_TEXTURES;
        work->scl.z = 1.0F;
        break;
    case CYCLE_RUN:
        beach_cycle_advance(task);
        return;
    case CYCLE_STILL:
        break;
    default:
        work->mode = CYCLE_INIT;
    }
}

void beach_act1_sand_step(struct task *task)
{
    struct task_work *work = task->work;
    int stage_act = stage_get_stage_act();

    if (stage_act != STAGE_ACT(STAGE_BEACH, 1)) {
        task_kill(task);
        return;
    }
    switch (work->mode) {
    case CYCLE_INIT:
        task->destructor = beach_cycle_destroy;
        task->draw = beach_cycle_advance;
        work->mode = CYCLE_RUN;
        /* the waves' first texture, past the sand's last. the first advance
         * wraps the tick to the sand's first */
        work->tick = ACT1_WAVE;
        VARS(work)->entries = beach_act1_sand_cycles;
        VARS(work)->count = ENTRIES(beach_act1_sand_cycles);
        VARS(work)->textures = SAND_TEXTURES;
        break;
    case CYCLE_RUN:
        beach_cycle_advance(task);
        return;
    case CYCLE_STILL:
        break;
    default:
        work->mode = CYCLE_INIT;
    }
}

void beach_act1_water_step(struct task *task)
{
    struct task_work *work = task->work;
    int stage_act = stage_get_stage_act();

    if (stage_act != STAGE_ACT(STAGE_BEACH, 1)) {
        task_kill(task);
        return;
    }
    switch (work->mode) {
    case CYCLE_INIT:
        task->destructor = beach_cycle_destroy;
        task->draw = beach_cycle_advance;
        work->mode = CYCLE_RUN;
        /* the sand's first texture. the tick counts up through the sand's
         * textures into the water's own, and cycles there */
        work->tick = ACT1_SAND;
        VARS(work)->entries = beach_act1_water_cycles;
        VARS(work)->count = ENTRIES(beach_act1_water_cycles);
        VARS(work)->textures = WATER_TEXTURES;
        break;
    case CYCLE_RUN:
        beach_cycle_advance(task);
        return;
    case CYCLE_STILL:
        break;
    default:
        work->mode = CYCLE_INIT;
    }
}

void beach_act2_wave_step(struct task *task)
{
    struct task_work *work = task->work;
    int stage_act = stage_get_stage_act();

    if (stage_act != STAGE_ACT(STAGE_BEACH, 2)) {
        task_kill(task);
        return;
    }
    switch (work->mode) {
    case CYCLE_INIT:
        task->destructor = beach_cycle_destroy;
        task->draw = beach_cycle_advance;
        work->mode = CYCLE_RUN;
        work->tick = ACT2_WAVE + WAVE_TEXTURES - 1;
        VARS(work)->entries = beach_act2_wave_cycles;
        VARS(work)->count = ENTRIES(beach_act2_wave_cycles);
        VARS(work)->textures = WAVE_TEXTURES;
        work->scl.z = 1.0F;
        break;
    case CYCLE_RUN:
        beach_cycle_advance(task);
        return;
    case CYCLE_STILL:
        break;
    default:
        work->mode = CYCLE_INIT;
    }
}

void beach_act2_sand_step(struct task *task)
{
    struct task_work *work = task->work;
    int stage_act = stage_get_stage_act();

    if (stage_act != STAGE_ACT(STAGE_BEACH, 2)) {
        task_kill(task);
        return;
    }
    switch (work->mode) {
    case CYCLE_INIT:
        task->destructor = beach_cycle_destroy;
        task->draw = beach_cycle_advance;
        work->mode = CYCLE_RUN;
        work->tick = ACT2_SAND + SAND_TEXTURES - 1;
        VARS(work)->entries = beach_act2_sand_cycles;
        VARS(work)->count = ENTRIES(beach_act2_sand_cycles);
        VARS(work)->textures = SAND_TEXTURES;
        break;
    case CYCLE_RUN:
        beach_cycle_advance(task);
        return;
    case CYCLE_STILL:
        break;
    default:
        work->mode = CYCLE_INIT;
    }
}

void beach_act2_water_step(struct task *task)
{
    struct task_work *work = task->work;
    int stage_act = stage_get_stage_act();

    if (stage_act != STAGE_ACT(STAGE_BEACH, 2)) {
        task_kill(task);
        return;
    }
    switch (work->mode) {
    case CYCLE_INIT:
        task->destructor = beach_cycle_destroy;
        task->draw = beach_cycle_advance;
        work->mode = CYCLE_RUN;
        work->tick = ACT2_WATER;
        VARS(work)->entries = beach_act2_water_cycles;
        VARS(work)->count = ENTRIES(beach_act2_water_cycles);
        VARS(work)->textures = WATER_TEXTURES;
        break;
    case CYCLE_RUN:
        beach_cycle_advance(task);
        return;
    case CYCLE_STILL:
        break;
    default:
        work->mode = CYCLE_INIT;
    }
}

/* the task's draw, which its step calls too. once every entries[0].delay
 * calls, work->tick moves to the cycle's next texture. each call sets every
 * entry's materials to work->tick. as the draw, it runs while the game is
 * paused */
static void beach_cycle_advance(struct task *task)
{
    struct task_work *work;
    /* @hack spelling: work assigned inside the entries' initializer, which
     * loads the entries (MOV.L @(8,R5),R13 at 0x0c9050d0) ahead of the timer's
     * increment; initialized in its own declaration, 23 of 74 words differ */
    struct cycle_entry *entries = VARS(work = task->work)->entries;
    struct cycle_entry *entry;
    NJS_OBJECT *object;
    int i;
    int j;

    work->timer++;
    if (work->timer == entries->delay) {
        work->tick++;
        if (work->tick >= VARS(work)->textures + entries->first) {
            work->tick = entries->first;
        }
        work->timer = 0;
    }
    for (i = 0; i < VARS(work)->count; i++) {
        entry = &entries[i];
        object = entry->object;
        for (j = 0; j < entry->count; j++) {
            object->model->mats[entry->materials[j]].attr_texId = work->tick;
        }
    }
}

static void beach_cycle_destroy(struct task *task)
{
}
