/* O TOROUT: the tornado that carries player 0 off at the end of act 0 */
#ifndef STG02_TORNADO_H
#define STG02_TORNADO_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* the object list's step for O TOROUT. task needs TASK_WORK's record, and
 * player 0's task must exist. as it starts, the tornado sets the wind blowing
 * at level 2 */
void windy_tornado_step(struct task *task);
/* windy_tornado_distance sets *distance to the distance from pos to the tornado
 * in x and z. windy_tornado_get_pos sets *pos to the tornado's position. each
 * returns TRUE once a tornado has run a step since STG02 loaded. each then
 * reads the work of the last tornado that did. before that, each returns
 * FALSE, with *distance 0 and *pos unwritten. nothing forgets the tornado as
 * its task is destroyed, so from then on each reads the freed work */
Bool windy_tornado_distance(NJS_POINT3 *pos, Float *distance);
Bool windy_tornado_get_pos(NJS_POINT3 *pos);
/* TRUE once the tornado has carried player 0 around it for 302 steps, which
 * ends act 0 */
Bool windy_tornado_done(void);

#endif
