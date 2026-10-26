/* the seconds E-102 gets, as a count rising in the world, facing the camera */
#ifndef PLAYER_E102_GETSEC_H
#define PLAYER_E102_GETSEC_H

struct task_work;

/* show seconds as a count rising from from->pos, on a task of its own */
void e102_getsec_create(struct task_work *from, int seconds);

#endif
