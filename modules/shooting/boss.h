/* the boss: BOSS, which flies the course behind the tornado */
#ifndef SHOOTING_BOSS_H
#define SHOOTING_BOSS_H

#include "system/task.h"

/* the object list's step for BOSS */
void boss_step(struct task *task);
/* breathe snow from the mouth the boss's flame comes from. nothing in this
 * image calls it */
void boss_frost_breathe(struct task *task);
/* turn work to face along dat_0caa414c's path, from point to the point
 * three ahead. point numbers a point of another course, as course_span_start
 * and course_span_end number the path's first and last. an ahead point at
 * course_span_end or past it wraps to course_span_start. a point outside the
 * span leaves work as it is. nothing in this image calls it */
void boss_face_other(struct task_work *work, int point);

#endif
