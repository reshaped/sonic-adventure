/* E-105 in Hot Shelter and the textures shared with its missiles */
#ifndef STG12_E105_H
#define STG12_E105_H
#include <ninja.h>
struct task;
/* the object list's step for E-105. task needs TASK_WORK's record, and
 * STG12 must stay loaded until E-105 and every task it makes are destroyed.
 * a failed allocation goes unchecked: the state, a sensor's task, a
 * missile's task or the head's record */
void e105_start(struct task *task);
extern NJS_TEXLIST e105_texlist;
#endif
