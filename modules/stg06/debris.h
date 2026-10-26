/* the debris, a model tree broken into pieces that fly apart. the whirl pulls a
 * player round, and sky_deck_hop lifts player 0 as a hit breaks a cannon */
#ifndef STG06_DEBRIS_H
#define STG06_DEBRIS_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* how a tree breaks. sky_deck_debris_create adds its own pos, ang, speed, accel
 * and floor to its copy. it adds the wind at pos to the copy's accel too */
struct debris_spec {
    /* the tree, with its siblings: a piece for each node, but none below a
     * node with NJD_EVAL_BREAK */
    NJS_OBJECT *object;
    NJS_POINT3 pos; /* the point the pieces burst away from */
    struct angles ang;
    NJS_VECTOR speed; /* added to every piece's first speed */
    NJS_VECTOR accel; /* added to a piece's speed each step */
    Float range; /* a flying piece is gone once farther than this from pos */
    /* only [1] is read. while it is above 0, it moves pieces that start near
     * the floor. a piece less than 0.1 above the floor starts at a height of
     * 0.1 instead. that height ignores the floor's, so it can put the piece
     * below the floor */
    Float unk_38[3];
    /* a random part of a piece's speed on each axis, up to this either way */
    Float scatter;
    /* a piece that starts d from pos bursts away from it. its burst speed is
     * burst_speed * 2 * d / burst_radius, with no cap. a piece spins faster
     * the farther from pos it starts. a burst_radius of 0 gives no burst */
    Float burst_radius;
    Float burst_speed;
    Float drag; /* the share of its speed a piece keeps each step */
    Uint32 flags; /* tested, and nothing done with it */
    /* any texlist but null sets obj_skydeck_texlist for the pieces' draw. the
     * list itself is never read */
    NJS_TEXLIST *texlist;
    Float floor; /* a piece that crosses it lands */
};

/* break spec's tree into flying pieces, in a task that ends once all are gone.
 * the tree stands at spec's pos plus pos, turned by the yaw of spec's ang plus
 * ang. a flying piece is gone once farther than range from the tree's pos. a
 * piece that crosses the floor lands, stops drawing, and is gone 41 steps
 * later. return the task, or null when task_create fails. the create copies
 * spec. with no room on the heap, it writes the record through null. it tests
 * the record only after that. the task then keeps a null record, which its step
 * and draw read through. the pieces' allocation goes untested */
struct task *sky_deck_debris_create(NJS_POINT3 *pos, struct angles *ang,
                                    NJS_VECTOR *speed, NJS_VECTOR *accel,
                                    Float floor, struct debris_spec *spec);
/* set player 0's speed to 2 straight up. raise by 1 the task that last hit
 * work's collision, which need not be player 0. that hit must be set */
void sky_deck_hop(struct task_work *work);
/* a whirl at pos that pulls the player nearest pos for steps + 1 steps. a point
 * radius from pos goes round it by turn each step. each step adds two pulls to
 * the player's accel, one toward the point and one toward pos. a pull is the
 * player's whole offset from its target. inside a band about the target, the
 * pull is cut to strength long, or half that toward pos. the band holds the
 * distances from twice strength to range. the whirl rises 10 each step the
 * player is within 10 of its height. null when task_create fails */
struct task *sky_deck_whirl_create(NJS_POINT3 *pos, Float radius, Float range,
                                   Angle turn, Sint16 steps, Float strength);

#endif
