/* sprite pools: many sprites that move and play alike, on one task */
#include <ninja.h>
#include <sg_xpt.h>

#include "effect/sprite.h"
#include "effect/sprite_pool.h"
#include "system/task.h"
#include "system/util.h"

void sprite_body_init(struct sprite_body *body, NJS_POINT3 *pos,
                      NJS_VECTOR *speed)
{
    if (pos != NULL) {
        VECTOR_COPY(&body->pos, pos);
    } else {
        body->pos.x = body->pos.y = body->pos.z = 0.0F;
    }
    if (speed != NULL) {
        VECTOR_COPY(&body->speed, speed);
    } else {
        body->speed.x = body->speed.y = body->speed.z = 0.0F;
    }
    body->scale = 1.0F;
    body->depth = 0.0F;
}

void sprite_body_move(struct sprite_body *body, struct sprite_motion *motion)
{
    VECTOR_SUM(&body->pos, &body->pos, &body->speed);
    body->speed.x -= body->speed.x * motion->drag;
    body->speed.y -= (body->speed.y * motion->drag) - motion->gravity;
    body->speed.z -= body->speed.z * motion->drag;
}

void sprite_pool_add(struct task *task, NJS_POINT3 *pos, NJS_VECTOR *speed,
                     Float age, Float depth)
{
    struct sprite_pool *pool;
    struct sprite_slot *slot;

    pool = task->extra;
    if (pool->count >= pool->capacity) {
        return;
    }
    pool->live[pool->count] = pool->free[pool->count];
    slot = &pool->slots[pool->live[pool->count]];
    VECTOR_COPY(&slot->body.pos, pos);
    VECTOR_COPY(&slot->body.speed, speed);
    slot->body.scale = 1.0F;
    slot->body.depth = depth;
    sprite_play_init(&slot->play);
    sprite_play_seek(&slot->play, pool->frames, age);
    pool->count++;
}

/* free the live sprite at index, and move the last live one into its place */
static void sprite_pool_remove(struct task *task, int index)
{
    struct sprite_pool *pool;

    pool = task->extra;
    if (pool->count > 0) {
        pool->count--;
        pool->free[pool->count] = pool->live[index];
        pool->live[index] = pool->live[pool->count];
    }
}

static void sprite_pool_draw(struct task *task)
{
    struct sprite_pool *pool;
    struct sprite_slot *slot;
    int i;

    pool = task->extra;
    for (i = 0; i < pool->count; i++) {
        slot = &pool->slots[pool->live[i]];
        sprite_render(&slot->body.pos, &slot->body, &slot->play, pool->frames);
    }
}

static Bool sprite_slot_step(struct sprite_slot *slot,
                             struct sprite_motion *motion,
                             const struct sprite_frame *frames)
{
    sprite_body_move(&slot->body, motion);
    if (sprite_play_step(&slot->play, frames) == SPRITE_END) {
        return FALSE;
    }
    return TRUE;
}

static void sprite_pool_step(struct task *task)
{
    struct sprite_pool *pool;
    struct sprite_slot *slot;
    int i;

    pool = task->extra;
    for (i = 0; i < pool->count; i++) {
        slot = &pool->slots[pool->live[i]];
        if (!sprite_slot_step(slot, &pool->motion, pool->frames)) {
            sprite_pool_remove(task, i--);
        }
    }
    sprite_pool_draw(task);
}

static void sprite_pool_step_own(struct task *task)
{
    struct sprite_pool *pool;
    struct sprite_slot *slot;
    int i;

    pool = task->extra;
    for (i = 0; i < pool->count; i++) {
        slot = &pool->slots[pool->live[i]];
        if (!pool->step(slot, &pool->motion, pool->frames)) {
            sprite_pool_remove(task, i--);
        }
    }
    sprite_pool_draw(task);
}

static void sprite_pool_destroy(struct task *task)
{
    struct sprite_pool *pool;

    pool = task->extra;
    heap_free(pool->free);
    heap_free(pool->live);
    heap_free(pool->slots);
}

struct task *sprite_pool_create(int count, struct sprite_motion *motion,
                                const struct sprite_frame *frames,
                                Bool (*step)(struct sprite_slot *slot,
                                             struct sprite_motion *motion,
                                             const struct sprite_frame *frames))
{
    struct task *task;
    struct sprite_pool *pool;
    int i;

    if ((task = task_create(TASK_WORK, 4, sprite_pool_step)) != NULL) {
        pool = heap_alloc(sizeof(*pool));
        if (pool != NULL) {
            task->extra = pool;
            pool->count = 0;
            pool->capacity = count;
            /* @hack type: signed products, which the image keeps in the frame
             * at 12 (ADD #-16,R15 at 0x8c0533fc); unsigned, the frame is 4
             * bytes smaller and 13 of 88 words differ */
            pool->free = heap_alloc(count * (int)sizeof(*pool->free));
            pool->live = heap_alloc(count * (int)sizeof(*pool->live));
            pool->slots = heap_alloc(count * (int)sizeof(*pool->slots));
            pool->motion = *motion;
            pool->frames = frames;
            pool->step = step;
            if (step != NULL) {
                task->step = sprite_pool_step_own;
            }
            for (i = 0; i < count; i++) {
                pool->free[i] = i;
            }
            task->draw = sprite_pool_draw;
            task->destructor = sprite_pool_destroy;
        } else {
            task_destroy(task);
            task = NULL;
        }
    }
    return task;
}
