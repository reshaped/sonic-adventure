/* O KAZB to O KAZG models that turn about z, and O KAZA, whose two parts move
 * apart and back on its action */
#ifndef STG09_KAZ_H
#define STG09_KAZ_H

#include "system/task.h"

/* the object list's steps. O KAZB to O KAZG turn right for an odd truncated
 * scl.x, and left otherwise. a model's angle is 150 times its truncated scl.y
 * times the steps it has run. the count of steps wraps after 65,535. the
 * product must fit an Angle up to that count, so scl.y must be under 219 in
 * magnitude. O KAZA's action starts at its frame scl.x. it moves 0.2 frames on
 * each step but a frame's first */
void kaz_a_step(struct task *task);
void kaz_b_step(struct task *task);
void kaz_c_step(struct task *task);
void kaz_d_step(struct task *task);
void kaz_e_step(struct task *task);
void kaz_f_step(struct task *task);
void kaz_g_step(struct task *task);

#endif
