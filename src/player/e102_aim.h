/* E-102's aim: the beam he sweeps, and his lock-ons on what it reaches */
#ifndef PLAYER_E102_AIM_H
#define PLAYER_E102_AIM_H
#include <sg_xpt.h>

struct e102;
struct player;
struct task;
struct task_work;

/* TRUE when task counts toward his bonus: it has a work and a collision,
 * and its first shape lacks LOCKON_NO_NUMBER */
Bool e102_target_counts(struct task *task);
/* a step of his aim, as e102->aim_mode says. an action button starts an
 * aim, and while one is held the beam goes out and he locks on to what it
 * reaches. once none is held, e102_shoot fires at his lock-ons. his cannon
 * opens while one is held, and closes after */
void e102_aim(struct task_work *work, struct player *record, struct e102 *e102);

#endif
