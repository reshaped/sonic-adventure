/* rings: a ring placed to be taken, and scattered rings, which fly out
 * from a point and lie a while before they go */
#ifndef OBJECT_RING_H
#define OBJECT_RING_H
#include <ninja.h>
#include <sg_xpt.h>

struct task;

/* a placed ring's work mode. RING_SPARKLE plays once the ring is taken.
 * RING_DONE ends the ring, once the sparkle has played or the ground has
 * moved away from under it */
enum ring_mode { RING_INIT, RING_WAIT, RING_SPARKLE, RING_DONE };

/* a placed ring's vars. spin is the first sparkle's frame once the ring is
 * taken. it starts at 0, and ring_attract adds 3 to it on each step player
 * 0 has a magnetic barrier. taking the ring doesn't set it back, so after 3
 * or more such steps the first sparkle starts past its last frame and never
 * draws */
struct ring_vars {
    Float spin;
    Sint8 bit; /* its bit in its group's placement state */
    Sint8 gone; /* 1 once it has left range */
};

#define RING(work) ((struct ring_vars *)&(work)->vars)

/* a ring placed with an scl.x of -1 follows the ground under it. it counts
 * down from this, and looks for the ground once its count passes 0: every
 * ninth step. successive rings start their counts one apart, so they look
 * on different steps */
#define RING_PROBE_COUNT 8

/* the count the next ring that follows the ground starts from, 0 to
 * RING_PROBE_COUNT. scattered rings and O IRON BALL JG's balls take their
 * counts from it too */
extern Sint32 ring_stagger;

/* the textures of a taken ring's sparkle, ring01 to ring08 */
extern NJS_TEXLIST ring_texlist;

/* set ring_stagger back to 0, as play in a stage starts */
void ring_reset_stagger(void);
/* the step of a ring a stage places, and of a ring another object makes */
void ring_step(struct task *task);
/* hit player. player 0 loses all its rings, and up to 20 of them scatter
 * from it. with no rings to lose, player 0 goes down instead. for any other
 * player the call does nothing */
void ring_lose(Sint8 player);
/* scatter count rings from the point x, y, z, their headings spread round
 * from a random one */
void ring_scatter(Float x, Float y, Float z, int count);

#endif
