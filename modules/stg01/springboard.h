/* O GOMBAN springboard, a board that bends under a player and springs
 * back */
#ifndef STG01_SPRINGBOARD_H
#define STG01_SPRINGBOARD_H
#include <ninja.h>
struct task;
/* the springboard's model. the frame's child is the board at rest, a strip of
 * 15 points. the child's sibling is the board's shape at a deflection of
 * -4.95. while the step draws the board bent, the child's model points at a
 * copy that blends the two */
extern NJS_OBJECT beach_springboard_object;
/* the object list's step for O GOMBAN, whose task needs a work. scl.x sets
 * how the board throws a player. 0 reflects the player's heading off the
 * board. 1 throws the player along one of four headings 45 degrees apart, by
 * the way it came in. 2 never throws. scl.y sets how hard the board pushes a
 * player back. at 1 the push grows as the stretch squared, and at 0 or 2 as
 * the stretch.
 *
 * scl.z, as a signed byte read unsigned, is the board's chain id, or 0 for
 * none. a board with an id joins a list of 20 as it starts, if a place is
 * free. it throws toward the first listed board whose id is one more. it does
 * so only when that board lies within 67.5 degrees of the throw's heading */
void beach_springboard_step(struct task *task);
#endif
