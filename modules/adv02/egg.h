/* the Chao egg the egg stand holds, which a player can carry to another act */
#ifndef ADV02_EGG_H
#define ADV02_EGG_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* texlists.c's pvm list loads its texture from MR_EGG */
extern NJS_TEXLIST egg_texlist;
/* the live egg's task, or null */
extern struct task *egg_task;
/* TRUE while the egg stand holds the egg, which then only draws */
extern Bool egg_in_stand;

/* the step the egg stand creates the egg's task with. the stand also runs it
 * once as it creates the task */
void egg_step(struct task *task);

#endif
