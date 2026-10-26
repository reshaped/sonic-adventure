/* the sai, a rhino on treads. it drives about until it sees a player, then
 * revs and charges at player 0. beach's, windy's and icecap's object lists
 * name it E SAITO */
#ifndef ENEMY_SAI_H
#define ENEMY_SAI_H
#include <ninja.h>

struct task;

/* the models enemy_break breaks a sai into, NULL-ended */
extern NJS_MODEL *sai_debris_models[];

/* the step the object lists name. while the set editor is open, it draws
 * the sai for the editor instead of moving it. otherwise its first call
 * sets the sai up and sets its draw */
void sai_step(struct task *task);

#endif
