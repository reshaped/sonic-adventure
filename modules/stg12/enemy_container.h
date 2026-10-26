/* O ENEMYCONTAINER: a container whose door opens as a player comes near */
#ifndef STG12_ENEMY_CONTAINER_H
#define STG12_ENEMY_CONTAINER_H

#include "system/task.h"

/* the object list's step. task needs TASK_WORK's and TASK_FORCE's records.
 * no code here reads the force, but player_collide_climb reads it unchecked
 * as Knuckles climbs the container */
void enemy_container_step(struct task *task);

#endif
