/* sprite pools: many sprites that move and play alike, on one task */
#ifndef EFFECT_SPRITE_POOL_H
#define EFFECT_SPRITE_POOL_H
#include <ninja.h>
#include <sg_xpt.h>

#include "effect/sprite.h"
#include "system/task.h"

/* a sprite in a pool, which holds the motion and frames they share */
struct sprite_slot {
    struct sprite_body body;
    struct sprite_play play;
};

/* a pool, as its task's extra. live[0] to live[count - 1] index the slots
 * in use, and free[count] to free[capacity - 1] the rest */
struct sprite_pool {
    Sint16 count;
    Sint16 capacity;
    Sint16 *free;
    Sint16 *live;
    struct sprite_slot *slots;
    /* the step each sprite runs in place of moving and playing, or null. it
     * returns FALSE once the sprite is done, which frees its slot */
    Bool (*step)(struct sprite_slot *slot, struct sprite_motion *motion,
                 const struct sprite_frame *frames);
    struct sprite_motion motion;
    const struct sprite_frame *frames;
};

/* set body at pos, moving at speed, with a scale of 1 and a depth of 0. a
 * null pos is the origin, and a null speed is still */
void sprite_body_init(struct sprite_body *body, NJS_POINT3 *pos,
                      NJS_VECTOR *speed);
/* move the body a step, then apply drag and gravity to its speed */
void sprite_body_move(struct sprite_body *body, struct sprite_motion *motion);
/* add a sprite to task's pool at pos, moving at speed, with depth and a
 * scale of 1. its play starts at a rate of 1, at age as sprite_play_seek
 * sets it. a full pool adds nothing. pos and speed are copied, and can't be
 * null */
void sprite_pool_add(struct task *task, NJS_POINT3 *pos, NJS_VECTOR *speed,
                     Float age, Float depth);
/* make a task that steps and draws a pool of up to count sprites. each
 * sprite moves by a copy of motion and plays frames, or runs step in their
 * place when step isn't null. a sprite whose play ends, or whose step
 * returns FALSE, leaves the pool. the task never ends on its own. returns
 * the task, or null when the task or the pool can't be allocated. the
 * pool's three arrays are allocated unchecked, and frames must outlive the
 * task */
struct task *sprite_pool_create(
    int count, struct sprite_motion *motion, const struct sprite_frame *frames,
    Bool (*step)(struct sprite_slot *slot, struct sprite_motion *motion,
                 const struct sprite_frame *frames));

#endif
