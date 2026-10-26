/* Eggman: his player task, on foot, and the edit mode that flies him */
#ifndef PLAYER_EGGMAN_H
#define PLAYER_EGGMAN_H

struct task;

/* Eggman's player task, which event scripts create with event_create_player */
void eggman_step(struct task *task);

#endif
