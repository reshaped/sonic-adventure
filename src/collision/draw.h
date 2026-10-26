/* a collision record's shapes, drawn in the colors of their attacks */
#ifndef COLLISION_DRAW_H
#define COLLISION_DRAW_H

struct collision_work;

/* draw each of the record's shapes but a bound, as a marker at the shape's
 * center in the color of its attack. collision may be null. a record with
 * no shapes, or with no task or task work, draws nothing */
void collision_draw(struct collision_work *collision);

#endif
