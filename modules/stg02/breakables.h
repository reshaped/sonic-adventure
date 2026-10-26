/* O BROOBJ and O BROCU objects that break into fading pieces */
#ifndef STG02_BREAKABLES_H
#define STG02_BREAKABLES_H

#include "system/task.h"

/* the object list's step for O BROOBJ, whose task needs a work. scl.x,
 * labeled ROTATE_SPEED in the set editor, turns its child that many degrees a
 * step */
void windy_breakable_step(struct task *task);
/* the object list's step for O BROCU, whose task needs a work. scl.x, labeled
 * PARTS, picks its size: 0 large, 1 medium and 2 small. scl.z, labeled BROKEN
 * ID, is its id, from 0 to 29. for E-102, the cube breaks once the targets of
 * id scl.y, labeled DYNAMAITE ID, are cleared */
void windy_breakable_cube_step(struct task *task);

#endif
