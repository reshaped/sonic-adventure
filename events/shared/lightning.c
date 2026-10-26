/* an event's lightning: sparks over a point, each a child task. a spark
 * waits a random while, then draws the eight lightning_frames at random
 * sizes, and waits again */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "draw/draw.h"
#include "draw/sprite.h"
#include "effect/hit.h" /* NOLINT(misc-include-cleaner) */
#include "shared/lightning.h"
#include "system/task.h"
#include "system/util.h"

/* what a spark's step does, by its work's mode */
#define SPARK_WAIT 0
#define SPARK_PLAY 1
#define SPARK_REST 2

/* the frames of lightning_frames a spark draws each time it plays */
#define SPARK_FRAMES 8

struct spark_vars {
    /* the steps left until it plays. the first delay can be 0, which runs
     * down from -1 and waits 2^32 steps */
    Sint32 delay;
    Sint32 frame; /* the next of lightning_frames it draws */
};

struct lightning_vars {
    Uint8 unk_00[4];
    Sint32 count; /* the sparks its first step makes */
};

#define SPARK(work) ((struct spark_vars *)&(work)->vars)
#define LIGHTNING(work) ((struct lightning_vars *)&(work)->vars)

static void event_lightning_destroy(struct task *task)
{
}

static void event_lightning_draw_spark(struct task_work *work)
{
    /* the sprite every spark draws, moved to each in turn */
    static NJS_SPRITE event_lightning_sprite = {
#include "shared/lightning/event_lightning_sprite.inc"
    };

    draw_set_material(1.0F, 1.0F, 1.0F, 1.0F);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    event_lightning_sprite.p.x = work->pos.x;
    event_lightning_sprite.p.y = work->pos.y;
    event_lightning_sprite.p.z = work->pos.z;
    event_lightning_sprite.sy = (rand() * (0.12F * RAND_SCALE)) + 0.05F;
    event_lightning_sprite.sx = event_lightning_sprite.sy;
    sprite_set_depth(-15.0F);
    sprite_draw_3d(&event_lightning_sprite, SPARK(work)->frame,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_SCALE | NJD_SPRITE_COLOR |
                       NJD_SPRITE_ANGLE);
    sprite_set_depth(0.0F);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}

/* a spark draws only from its step. while it plays, each step counts tick
 * up. it draws its next frame when tick comes to 9, 11, 13 or 15 mod 16 */
static void event_lightning_spark_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case SPARK_WAIT:
        SPARK(work)->delay--;
        if (SPARK(work)->delay == 0) {
            work->mode = SPARK_PLAY;
        }
        break;
    case SPARK_PLAY:
        work->tick++;
        if ((work->tick & 8) && (work->tick & 1)) {
            if (SPARK(work)->frame > SPARK_FRAMES - 1) {
                SPARK(work)->frame &= SPARK_FRAMES - 1;
                work->mode = SPARK_REST;
            } else {
                event_lightning_draw_spark(work);
                SPARK(work)->frame++;
            }
        }
        break;
    case SPARK_REST:
        SPARK(work)->delay = RANDOM(60.0F, 140.0F);
        work->mode = SPARK_WAIT;
        break;
    }
}

static void event_lightning_step(struct task *task)
{
    task_step_children(task);
}

static void event_lightning_init(struct task *task)
{
    struct task_work *work = task->work;
    struct task *spark;
    int i;

    for (i = 0; i < LIGHTNING(work)->count; i++) {
        spark = task_create_child(TASK_WORK, event_lightning_spark_step, task);
        spark->work->pos.x = work->pos.x + RANDOM(-2.5F, 2.5F);
        spark->work->pos.y = work->pos.y + RANDOM(8.0F, 13.0F);
        spark->work->pos.z = work->pos.z + RANDOM(-2.5F, 2.5F);
        spark->work->mode = SPARK_WAIT;
        spark->work->tick = 0;
        SPARK(spark->work)->frame = 0;
        SPARK(spark->work)->delay = rand() * (120.0F * RAND_SCALE);
    }
    task->step = event_lightning_step;
    task->destructor = event_lightning_destroy;
    event_lightning_step(task);
}

/* the lightning event_lightning_create made last, until
 * event_lightning_kill */
static struct task *event_lightning_task = NULL;

void event_lightning_create(Float x, Float y, Float z, int count)
{
    struct task *task = task_create(TASK_WORK, 1, event_lightning_init);

    if (task != NULL) {
        task->work->pos.x = x;
        task->work->pos.y = y;
        task->work->pos.z = z;
        LIGHTNING(task->work)->count = count;
        event_lightning_task = task;
    }
}

void event_lightning_kill(void)
{
    if (event_lightning_task != NULL) {
        task_kill(event_lightning_task);
        event_lightning_task = NULL;
    }
}
