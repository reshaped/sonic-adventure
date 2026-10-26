/* the robo, a flying enemy that sends its grabber after player 0, then
 * fetches it. final's and shelter's object lists name it E ROBO */
#ifndef ENEMY_ROBO_H
#define ENEMY_ROBO_H
#include <ninja.h>

struct task;

/* the texture list the PVM E_ROBO fills, which final and shelter load */
extern NJS_TEXLIST e_robo_texlist;

/* the step the object lists name. it sets the robo up, then hands the task
 * to the robo's own step, draw and destructor */
void robo_init(struct task *task);

#endif
