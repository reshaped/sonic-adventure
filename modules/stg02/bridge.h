/* O TORBRI: bridge sections that break apart as the tornado nears. the
 * tornado also throws sections, which break near the point they fly at */
#ifndef STG02_BRIDGE_H
#define STG02_BRIDGE_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* the vars of a section the tornado throws, whose creator sets reach and
 * may set still. the creator sets the section's start in work->pos. it sets
 * the section's mark, the point it flies at, in work->scl */
struct debris_vars {
    Float speed; /* its distance a step at the start, 1 up to 2 at random */
    Sint8 still; /* TRUE holds the mark still. otherwise the mark rises */
    Float reach; /* the section breaks once it is nearer its mark than this */
};

/* a piece's vars. its creator sets node, and thrown for a thrown piece */
struct piece_vars {
    Float heading; /* the way a circling piece moves, in degrees about y */
    NJS_OBJECT *node; /* its model, and its offset from what broke */
    /* @hack type: thrown, an array of one, so SHC adds its offset to the
     * work's address and stores through the sum (ADD #16,R2 at 0x0c90579e).
     * as a plain Sint8, summed over windy_bridge_piece_start,
     * windy_bridge_piece_step, windy_bridge_break, windy_bridge_break_at and
     * windy_rock_step, 828 of 1309 words differ */
    Sint8 thrown[1];
};

#define DEBRIS(work) ((struct debris_vars *)&(work)->vars)
#define PIECE(work) ((struct piece_vars *)&(work)->vars)

/* the object list's step for O TORBRI. scl.x picks the section's model, 0
 * to 3, and any other value picks the first. a section whose scl.y is below
 * 0 rises before it breaks */
void windy_bridge_start(struct task *task);
/* the first step of a piece, a child of the section or rock that broke. its
 * creator sets its pos. a thrown piece flies out along its parent's scl.x
 * and scl.z, then falls. any other flies up, then circles */
void windy_bridge_piece_start(struct task *task);
/* break a bridge section at pos: make task a child piece for each model a
 * section breaks into. each starts at pos plus its model's offset, with
 * task's angles. thrown TRUE makes them thrown pieces. the dust puffs at
 * task's own position, not at pos */
void windy_bridge_break_at(NJS_POINT3 *pos, int thrown, struct task *task);
/* task step for a section the tornado throws. it flies from work->pos at its
 * mark, and breaks into thrown pieces within reach of it. once its pieces
 * have run, it destroys them, but the task itself never ends */
void windy_bridge_debris_step(struct task *task);

#endif
