/* debris pieces that tumble and bounce, trailing smoke or flame */
#ifndef STG05_DEBRIS_H
#define STG05_DEBRIS_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* the record at a piece's task->motion */
struct debris_motion {
    void *unk_00;
    NJS_VECTOR speed;
    Uint8 unk_10[24];
    struct angles spin; /* the turn the piece takes each step */
    Float radius; /* its model's radius at its scale, which sizes its puffs */
};

/* what a piece is and does. debris_create's caller supplies it, and each
 * piece keeps a pointer to it */
struct debris_kind {
    /* the piece's model, 0 to 5 for debris_0_object to debris_5_object.
     * at 6 or 7, debris_create reads the model's radius through null. above
     * 7, it reads out of bounds */
    Uint8 model;
    Uint8 flags; /* DEBRIS_PUFFS and the rest */
    Uint8 puff; /* DEBRIS_SMOKE or DEBRIS_FLAME */
    Uint8 unk_03;
    Uint16 puff_steps; /* the steps from one puff to the next */
    Uint16 life; /* the steps a piece lasts, or DEBRIS_FOREVER */
    Float bounce; /* the share of its vertical speed a bounce keeps */
    /* the most a piece turns about each axis a step, either way */
    struct angles spin;
    /* the kind's own start, step and destructor, each of which may be NULL.
     * start runs on a piece's first step. step runs at the end of each later
     * step that doesn't kill the piece. destroy runs as its task ends */
    void (*start)(struct task_work *work, struct debris_motion *motion);
    void (*step)(struct task_work *work, struct debris_motion *motion);
    void (*destroy)(struct task_work *work, struct debris_motion *motion);
};

/* in a kind's flags */
#define DEBRIS_PUFFS 0x1 /* a piece trails a puff every puff_steps steps */
#define DEBRIS_BOUNCES 0x2 /* a piece bounces off the floor */
#define DEBRIS_SHADOW 0x4 /* a piece casts a shadow on the floor */

/* in a kind's puff */
#define DEBRIS_SMOKE 1
#define DEBRIS_FLAME 2

/* a life meant to keep a piece forever. debris_step's test for it never
 * holds, so such a piece counts down and lasts 65535 steps */
#define DEBRIS_FOREVER 0xffff

/* create a piece of kind at pos, moving at speed, at scale. its spin about
 * each axis is random, within kind->spin. pos and speed are copied, but the
 * piece keeps kind, which must outlive it. nothing is made when task_create
 * fails. nothing in the module calls debris_create */
void debris_create(NJS_POINT3 *pos, NJS_VECTOR *speed, Float scale,
                   struct debris_kind *kind);

#endif
