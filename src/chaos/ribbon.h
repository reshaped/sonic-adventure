/* the beads of light that run along strands in a Chaos's body */
#ifndef CHAOS_RIBBON_H
#define CHAOS_RIBBON_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

struct chaos;

/* a frame the beads of its strands draw in: node's frame in the boss's
 * pose, moved by pos and then turned by ang */
struct ribbon_node {
    Uint16 node;
    NJS_POINT3 pos;
    struct angles ang;
};

struct ribbon_strand {
    Sint16 node; /* the index of the ribbon node it lies in */
    Sint8 count; /* the points in path */
    /* the points its beads run along, or null for a strand that starts
     * none */
    NJS_POINT3 *path;
};

struct ribbon {
    Sint8 node_count;
    Sint8 strand_count;
    struct ribbon_strand *strands;
    struct ribbon_node *nodes;
    Float scale; /* scales the beads' size and speed, 0 meaning 1 */
};

extern NJS_ARGB ribbon_color; /* the color the beads draw in */
/* by ribbon node, nonzero to hide the beads on its strands */
extern Sint8 ribbon_hidden[];
/* the sprite the beads draw, set up by ribbon_init. each draw sets its
 * scale first */
extern NJS_SPRITE ribbon_sprite;

/* start beads as ribbon_spawn does, then place the nodes as
 * ribbon_place_nodes does. its sine keeps a phase of its own */
void ribbon_update(void);
/* start beads on strands picked at random. a call starts a random count of
 * them, below (6 + the extra rate) * |sin phase|, and phase moves a degree
 * each call. task_create's result is used unchecked */
void ribbon_spawn(void);
/* set each ribbon node's frame, as struct ribbon_node gives it, on the
 * current matrix. the last pose must reach every node, unchecked */
void ribbon_place_nodes(void);
/* set up the one ribbon: show every node, and set the extra rate to 0. owner
 * and ribbon's nodes, strands and paths are kept, not copied, until
 * ribbon_free. work is kept unread. ribbon must hold a strand, and each
 * strand's node must be below the node count and 64, unchecked. the frames'
 * allocation is unchecked too. free the ribbon set up before first */
void ribbon_init(struct task_work *work, struct chaos *owner,
                 struct ribbon *ribbon);
/* free the nodes' frames, after the last bead is gone. the pointer isn't
 * cleared: call it once for each ribbon_init, and start no beads and place
 * no nodes until the next */
void ribbon_free(void);
/* set the extra rate, which ribbon_spawn and ribbon_update add to 6. the
 * events with Chaos set it */
void ribbon_set_extra_rate(Float rate);

#endif
