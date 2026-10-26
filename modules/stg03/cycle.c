/* act 1's texture cycle, which steps four models' materials through a run
 * of textures */
#include <ninja.h>
#include <sg_xpt.h>

#include "stage/stage.h"
#include "stg03/cycle.h"
#include "system/task.h"

extern NJS_OBJECT cycle_object_0;
extern NJS_OBJECT cycle_object_1;
extern NJS_OBJECT cycle_object_2;
extern NJS_OBJECT cycle_object_3;

/* the cycle steps through CYCLE_TEXTURES texture numbers from CYCLE_FIRST */
#define CYCLE_FIRST 74
#define CYCLE_TEXTURES 14
#define CYCLE_MODELS 4

/* a model whose materials take the cycle's texture. twinkle_cycle_advance
 * reads first and delay from the first entry alone */
struct cycle_entry {
    NJS_OBJECT *object;
    Uint8 count; /* the entries of materials in use */
    Uint8 materials[3]; /* indices into the model's mats */
    Uint8 first; /* the cycle's first texture */
    Uint8 unk_09;
    Sint16 delay; /* the calls of twinkle_cycle_advance each texture lasts */
};

/* the cycle's work->vars. work->tick holds the texture shown, and
 * work->timer the calls of twinkle_cycle_advance since it changed */
struct cycle_vars {
    struct cycle_entry *entries;
    Sint8 count;
    Sint8 textures; /* the cycle's length */
};

#define VARS(work) ((struct cycle_vars *)&(work)->vars)

enum cycle_mode { CYCLE_INIT, CYCLE_RUN, CYCLE_STILL };

static void twinkle_cycle_advance(struct task *task);
static void twinkle_cycle_destroy(struct task *task);

static struct cycle_entry twinkle_cycles[CYCLE_MODELS] = {
#include "stg03/cycle/twinkle_cycles.inc"
};

void twinkle_cycle_step(struct task *task)
{
    struct task_work *work = task->work;
    int stage_act = stage_get_stage_act();

    if (stage_act != STAGE_ACT(STAGE_TWINKLE, 1)) {
        task_kill(task);
        return;
    }
    switch (work->mode) {
    case CYCLE_INIT:
        task->destructor = twinkle_cycle_destroy;
        task->draw = twinkle_cycle_advance;
        work->mode = CYCLE_RUN;
        work->tick = CYCLE_FIRST;
        VARS(work)->entries = twinkle_cycles;
        VARS(work)->count = CYCLE_MODELS;
        VARS(work)->textures = CYCLE_TEXTURES;
        break;
    case CYCLE_RUN:
        twinkle_cycle_advance(task);
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
static void twinkle_cycle_advance(struct task *task)
{
    struct task_work *work = task->work;
    struct cycle_entry *entries = VARS(work)->entries;
    struct cycle_entry *entry;
    NJS_OBJECT *object;
    Uint32 i;
    Uint32 j;

    work->timer++;
    if (work->timer == entries->delay) {
        work->tick++;
        if (work->tick >= VARS(work)->textures + entries->first) {
            work->tick = entries->first;
        }
        work->timer = 0;
    }
    for (i = 0; i < (Uint32)VARS(work)->count; i++) {
        entry = &entries[i];
        object = entry->object;
        for (j = 0; j < entry->count; j++) {
            object->model->mats[entry->materials[j]].attr_texId = work->tick;
        }
    }
}

static void twinkle_cycle_destroy(struct task *task)
{
}
