/* the course's texture cycles, each a material that shows textures in turn */
#include <ninja.h>
#include <sg_xpt.h>

#include "al_race/cycle.h"
#include "al_race/scenery.h"
#include "system/task.h"
#include "system/task_desc.h"

/* the course's materials that cycle */
extern NJS_MATERIAL dat_0cbbfb88;
extern NJS_MATERIAL dat_0cbbfeb4;
extern NJS_MATERIAL dat_0cbab120;

/* cycle_start's arg: a material, and the textures it shows in turn */
struct cycle_kind {
    NJS_MATERIAL *material;
    const Sint16 *textures;
    Sint16 count;
    Sint16 steps; /* the steps each texture shows */
};

/* a cycle's state, in its task's motion */
struct cycle {
    Sint16 texture; /* an index into textures */
    Sint16 count;
    Sint16 timer; /* the steps since the last change */
    Sint16 steps;
    NJS_MATERIAL *material;
    const Sint16 *textures;
};

#define CYCLE(task) ((struct cycle *)(task)->motion)

#define CYCLE_COUNT 3

static void cycle_start(struct task *task, void *arg);
static void cycle_step(struct task *task);
static void cycle_show(struct cycle *cycle);

static const struct task_desc cycle_desc = {
    sizeof(struct cycle), 0, cycle_start, cycle_step, NULL, NULL};
static const Sint16 cycle_textures[14] = {
#include "al_race/cycle/cycle_textures.inc"
};
static const struct cycle_kind cycle_kinds[CYCLE_COUNT] = {
#include "al_race/cycle/cycle_kinds.inc"
};

void cycles_create(void)
{
    const struct cycle_kind *kind;
    /* @hack spelling: count, CYCLE_COUNT in a local, which keeps the loop
     * rolled, as the image does. as CYCLE_COUNT, SHC writes the body three
     * times a pass, the function runs 16 bytes long, and 15 of 22 words
     * differ */
    int count = CYCLE_COUNT;
    int i;

    for (i = 0, kind = cycle_kinds; i < count; i++, kind++) {
        scenery_add(&cycle_desc, (void *)kind);
    }
}

static void cycle_start(struct task *task, void *arg)
{
    struct cycle *cycle = CYCLE(task);

    /* @hack spelling: the kind read through a cast of arg at each use, which
     * SHC reads from R5, where arg arrives (0x0cb8a9b4 to 0x0cb8a9c6), as the
     * image does. through a local of the kind's type, SHC copies arg into
     * R4, the function runs 2 bytes long, and 13 of 17 words differ */
    cycle->texture = 0;
    cycle->count = ((struct cycle_kind *)arg)->count;
    cycle->timer = 0;
    cycle->steps = ((struct cycle_kind *)arg)->steps;
    cycle->material = ((struct cycle_kind *)arg)->material;
    cycle->textures = ((struct cycle_kind *)arg)->textures;
    cycle_show(cycle);
}

static void cycle_step(struct task *task)
{
    struct cycle *cycle = CYCLE(task);

    cycle->timer++;
    if (cycle->timer >= cycle->steps) {
        cycle->timer = 0;
        cycle->texture++;
        if (cycle->texture >= cycle->count) {
            cycle->texture = 0;
        }
        cycle_show(cycle);
    }
}

static void cycle_show(struct cycle *cycle)
{
    cycle->material->attr_texId = (Uint16)cycle->textures[cycle->texture];
}
