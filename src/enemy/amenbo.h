/* the amenbo, a water strider that rows across the water. beach's and
 * twinkle's object lists name it E AMEMB */
#ifndef ENEMY_AMENBO_H
#define ENEMY_AMENBO_H
#include <ninja.h>

struct task;

/* the texture list the PVM E_AMENBO fills, which beach and twinkle load */
extern NJS_TEXLIST e_amenbo_texlist;

/* the step the object lists name. it sets the amenbo up, then hands the
 * task to the amenbo's own step, draw and destructor */
void amenbo_start(struct task *task);

#endif
