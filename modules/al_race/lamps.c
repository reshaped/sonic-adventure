/* Chao race lamp rows that run through a pattern together */
#include <ninja.h>
#include <sg_xpt.h>

#include "al_race/lamps.h"
#include "al_race/scenery.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/task_desc.h"
#include "system/texture.h"

/* the rows' objects, and each row's model's materials, one for each lamp */
extern NJS_OBJECT dat_0cbc8e00;
extern NJS_OBJECT dat_0cbc926c;
extern NJS_OBJECT dat_0cbc96d8;
extern NJS_OBJECT dat_0cbcaf20;
extern NJS_MATERIAL dat_0cbc89c8[];
extern NJS_MATERIAL dat_0cbc8e34[];
extern NJS_MATERIAL dat_0cbc92a0[];
extern NJS_MATERIAL dat_0cbca838[];

/* lamps in a row */
#define LAMPS 8

struct lamp_row {
    NJS_OBJECT *object;
    NJS_MATERIAL *lamps[LAMPS];
};

/* a set of rows' starting values */
struct lamps_kind {
    const struct lamp_row *rows;
    Sint16 count;
    /* a lit lamp's texture is textures[1][lamp], and an unlit one's
     * textures[0][lamp] */
    const Uint16 (*textures)[LAMPS];
    const Uint8 *patterns; /* a byte for each pattern, a bit for each lamp */
    Sint16 pattern_count;
    Sint16 steps; /* how long each pattern shows */
};

/* the rows' state, in their task's motion */
struct lamps {
    Sint16 pattern;
    Sint16 timer; /* the steps until the next pattern */
    /* the rows' count. it and the fields after it copy the lamps_kind's */
    Sint16 count;
    Sint16 pattern_count;
    Sint16 steps;
    const struct lamp_row *rows;
    const Uint16 (*textures)[LAMPS];
    const Uint8 *patterns;
};

#define LAMP_ROWS(task) ((struct lamps *)(task)->motion)

static void lamps_start(struct task *task, void *arg);
static void lamps_step(struct task *task);
static void lamps_draw(struct task *task);
static void lamps_show(struct lamps *lamps);

static const struct task_desc lamps_desc = {
    sizeof(struct lamps), TASK_WORK, lamps_start, lamps_step, lamps_draw, NULL};
static const Uint16 lamp_textures[2][LAMPS] = {
#include "al_race/lamps/lamp_textures.inc"
};
/* the patterns: lamps out from the middle, alternate lamps, a chase that fills,
 * a lamp that runs and stacks up, and all the lamps flashing */
static const Uint8 lamp_patterns[71] = {
#include "al_race/lamps/lamp_patterns.inc"
};
static const struct lamp_row hall_rows[3] = {
    {&dat_0cbc8e00,
     {&dat_0cbc89c8[0], &dat_0cbc89c8[1], &dat_0cbc89c8[2], &dat_0cbc89c8[3],
      &dat_0cbc89c8[6], &dat_0cbc89c8[7], &dat_0cbc89c8[5], &dat_0cbc89c8[4]}},
    {&dat_0cbc926c,
     {&dat_0cbc8e34[0], &dat_0cbc8e34[1], &dat_0cbc8e34[2], &dat_0cbc8e34[3],
      &dat_0cbc8e34[6], &dat_0cbc8e34[7], &dat_0cbc8e34[5], &dat_0cbc8e34[4]}},
    {&dat_0cbc96d8,
     {&dat_0cbc92a0[0], &dat_0cbc92a0[1], &dat_0cbc92a0[2], &dat_0cbc92a0[3],
      &dat_0cbc92a0[6], &dat_0cbc92a0[7], &dat_0cbc92a0[5], &dat_0cbc92a0[4]}},
};
static const struct lamps_kind hall_lamps = {
#include "al_race/lamps/hall_lamps.inc"
};
static const struct lamp_row course_rows[1] = {
    {&dat_0cbcaf20,
     {&dat_0cbca838[4], &dat_0cbca838[5], &dat_0cbca838[7], &dat_0cbca838[6],
      &dat_0cbca838[3], &dat_0cbca838[2], &dat_0cbca838[1], &dat_0cbca838[0]}},
};
static const struct lamps_kind course_lamps = {
#include "al_race/lamps/course_lamps.inc"
};

void hall_lamps_create(void)
{
    scenery_add(&lamps_desc, (void *)&hall_lamps);
}

void course_lamps_create(void)
{
    scenery_add(&lamps_desc, (void *)&course_lamps);
}

static void lamps_start(struct task *task, void *arg)
{
    const struct lamp_row *rows = ((struct lamps_kind *)arg)->rows;
    int steps = ((struct lamps_kind *)arg)->steps;
    struct task_work *work = task->work;
    struct lamps *lamps = LAMP_ROWS(task);

    scenery_place(work, rows->object);
    lamps->pattern = 0;
    lamps->timer = steps;
    lamps->count = ((struct lamps_kind *)arg)->count;
    lamps->pattern_count = ((struct lamps_kind *)arg)->pattern_count;
    lamps->steps = steps;
    lamps->rows = rows;
    lamps->textures = ((struct lamps_kind *)arg)->textures;
    lamps->patterns = ((struct lamps_kind *)arg)->patterns;
}

static void lamps_step(struct task *task)
{
    struct lamps *lamps;

    lamps_draw(task);
    lamps = LAMP_ROWS(task);
    lamps->timer--;
    if (lamps->timer <= 0) {
        lamps->timer = lamps->steps;
        lamps->pattern++;
        if (lamps->pattern >= lamps->pattern_count) {
            lamps->pattern = 0;
        }
    }
}

static void lamps_draw(struct task *task)
{
    struct lamps *lamps;
    const struct lamp_row *row;
    int count;
    int i;

    if (frame_step == 0 && !placement_far_from_camera(task)) {
        lamps = LAMP_ROWS(task);
        count = lamps->count;
        lamps_show(lamps);
        texture_set_stage();
        for (i = 0, row = lamps->rows; i < count; i++, row++) {
            draw_tree(row->object);
        }
    }
}

/* give each row's lamps the current pattern's textures. the pattern's
 * lowest bit picks the last lamp's, and its highest bit the first's */
static void lamps_show(struct lamps *lamps)
{
    int count = lamps->count;
    const Uint16(*textures)[LAMPS] = lamps->textures;
    Uint8 pattern = lamps->patterns[lamps->pattern];
    const struct lamp_row *row;
    int i;
    int j;
    Uint8 bits;

    for (i = 0, row = lamps->rows; i < count; i++, row++) {
        NJS_MATERIAL *const *lamp;

        bits = pattern;
        j = LAMPS;
        lamp = &row->lamps[LAMPS];
        while (j > 0) {
            j--;
            --lamp;
            (*lamp)->attr_texId = textures[bits & 1][j];
            bits >>= 1;
        }
    }
}
