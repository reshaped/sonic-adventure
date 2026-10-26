/* a scene's zako: a zako that flies its scene's path with a gun on its back,
 * and the gun's shots */
#ifndef SHOOTING_SCENE_ZAKO_H
#define SHOOTING_SCENE_ZAKO_H

#include "system/task.h"

/* the step of a SCENE_ZAKO actor, and of any kind without its own */
void scene_zako_step(struct task *task);
/* aim work at player 0. work->scl becomes a gun shot's velocity toward it,
 * and work->ang.x and ang.y face it */
void scene_gun_aim(struct task_work *work);
/* the step of a gun's shot, which must be a child of the gun's task. the
 * shot starts at the gun's pos, ang and scl, and flies at scl for 120
 * steps. it has no collision, so it hurts nothing */
void scene_shot_step(struct task *task);

#endif
