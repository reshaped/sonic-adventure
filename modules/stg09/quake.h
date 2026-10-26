/* a quake: the camera bobs like a settling spring, and pad 0 vibrates */
#ifndef STG09_QUAKE_H
#define STG09_QUAKE_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* start a quake if player 0 is nearer pos than radius. its spring starts at
 * speed, and pad 0 vibrates. it returns the quake's task, or NULL when
 * player 0 is farther or no task can be made. player 0 must exist. a shipped
 * bug computes the strength from FR15 as the caller leaves it. called from
 * lion_slam, the quake never ends */
struct task *quake_start(NJS_POINT3 *pos, Float radius, Float speed);

#endif
