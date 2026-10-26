/* the leon, a chameleon that stalks player 0 unseen. windy's, ruin's and
 * shelter's object lists name it E LEON */
#ifndef ENEMY_LEON_H
#define ENEMY_LEON_H
#include <ninja.h>

struct lockon_target;
struct task;
struct task_work;

/* the texture list the PVM E_LEON fills, which windy, ruin and shelter
 * load */
extern NJS_TEXLIST e_leon_texlist;

/* the step the object lists name. it sets the leon up, then hands the task
 * to the leon's own step and destructor */
void leon_start(struct task *task);
/* enemy_collide, for the leon. leon.c's steps expand each call */
void leon_collide(struct task_work *work, struct lockon_target *record);

#endif
