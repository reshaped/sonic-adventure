/* trees of models broken into fragments that fly apart, and a knock-back of
 * player 0 from an object */
#ifndef STG09_FRAGMENTS_H
#define STG09_FRAGMENTS_H

#include <ninja.h>

#include "system/task.h"

/* how trees of models break apart. fragments_scatter adds its own arguments
 * to center, ang, speed, accel and ground in its copy */
struct fragments_spec {
    NJS_OBJECT *objects; /* the first of the sibling trees to break */
    NJS_POINT3 center; /* the point the fragments burst out from */
    struct angles ang; /* the trees' angles, turned about z, y, then x */
    /* each fragment's speed starts as this plus its burst and spread */
    NJS_VECTOR speed;
    NJS_VECTOR accel; /* added to each fragment's speed every step */
    Float reach; /* a flying fragment farther than this from center is gone */
    Float unk_38[3];
    /* the most each axis of a fragment's speed differs at random, either
     * way */
    Float spread;
    /* a fragment bursts out from center at 2 * burst_speed for each
     * burst_radius of its distance. a burst_radius of 0 gives no burst */
    Float burst_radius;
    Float burst_speed;
    Float drag; /* the share of its speed a fragment keeps each step */
    /* a texlist the fragments load as they start and release as they end,
     * or NULL */
    NJS_TEXLIST *own_texlist;
    NJS_TEXLIST *texlist; /* set before the fragments draw, or NULL */
    Float ground; /* the height the fragments land at */
    /* the sizes of a fragment's flame and dust. each adds smoke_scale for
     * each unit of the model's radius to a share of smoke_size. the flame,
     * once the fragment lands, takes all of smoke_size and is then doubled.
     * a puff of dust, as the fragment flies, takes a random share */
    Float smoke_size;
    Float smoke_scale;
};

/* when player 0 is the player nearest work, turn it away from work and send
 * it off. it goes 1.5 ahead and 1.5 up, or 1 and 1 for the slow one. its
 * stick is ignored for 60 steps, and it starts 1 higher. player 0 must
 * exist */
void casino_knock_player_back(struct task_work *work);
void casino_knock_player_back_slow(struct task_work *work);

/* break spec's trees into fragments, one for each node but those below a
 * node with NJD_EVAL_BREAK. returns the task that moves and draws them, or
 * NULL when no task is free. it plays the break's sound and vibrates pad 0.
 * the task frees the fragments once all are gone, and the heap must have
 * room for them */
struct task *fragments_scatter(NJS_POINT3 *pos, struct angles *ang,
                               NJS_VECTOR *speed, NJS_VECTOR *accel,
                               struct fragments_spec *spec, Float ground);

#endif
