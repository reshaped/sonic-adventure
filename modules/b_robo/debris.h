/* debris: Zero's model, posed, flying apart node by node */
#ifndef B_ROBO_DEBRIS_H
#define B_ROBO_DEBRIS_H
#include <ninja.h>
#include <sg_xpt.h>

/* break the action's tree, as posed at frame, into a piece for each node.
 * a piece waits up to 60 steps, then flies out from origin. about 1 piece
 * in 60 waits for none, and falls straight down from its pose instead.
 * origin is copied, but the action's object is kept and must outlive the
 * debris */
void erobo_debris_create(NJS_POINT3 *origin, NJS_ACTION *action, Float frame);

#endif
