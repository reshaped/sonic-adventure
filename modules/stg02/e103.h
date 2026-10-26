/* E-103, and the texlist its missiles share */
#ifndef STG02_E103_H
#define STG02_E103_H
#include <ninja.h>
struct task;
/* the object list's step for E E-103. task needs TASK_WORK's record, and
 * player 0's task must exist. STG02 must stay loaded until E-103 and every
 * task it makes are destroyed.
 *
 * no allocation is checked. without its motion record, e103_start writes
 * through null. without its shapes' copy, E-103 writes through null as it
 * first guards. without its lock-on record, enemy_alloc_record kills the
 * task, but e103_start sets the task's step after that. E-103 then runs on,
 * and reads through null when a hit hurts it */
void e103_start(struct task *task);
extern NJS_TEXLIST windy_e103_texlist;
#endif
