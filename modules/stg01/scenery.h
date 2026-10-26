/* Emerald Coast's scenery: models in the collision world, solid models, a
 * scaled model, and a splash step */
#ifndef STG01_SCENERY_H
#define STG01_SCENERY_H

#include "system/task.h"

/* the object list's steps. O BFLOOT's, O BUNGA's, O ROCK's and O IWAPO's
 * call model_start_world. O SEKITYU's and O Rock2's call model_start_solid,
 * with a collision shape of their own outside the set editor. O KOMOMO's
 * calls model_start_scaled */
void beach_float_step(struct task *task);
void bungalow_step(struct task *task);
void beach_rock_step(struct task *task);
void rock_pole_step(struct task *task);
void stone_pillar_step(struct task *task);
void komomo_step(struct task *task);
void beach_rock_2_step(struct task *task);
/* a step that throws a splash at the task's position every 17 steps. no
 * word in the image or in 1ST_READ.BIN holds its address */
void beach_splash_step(struct task *task);

#endif
