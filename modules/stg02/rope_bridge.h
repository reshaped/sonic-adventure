/* O TURI1, O TURIBR, O TURIBR2 and O E102BR: rope bridges that bridge_init
 * builds. O STBRID and O YURE: models */
#ifndef STG02_ROPE_BRIDGE_H
#define STG02_ROPE_BRIDGE_H

#include "system/task.h"

/* the object list's steps. task needs TASK_WORK's record, and a bridge's
 * task needs TASK_FORCE's too. each bridge writes through null when
 * bridge_init finds no room. it needs room in the heap and in the collision
 * world's pool of objects */

/* O TURI1: a bridge that stands */
void windy_rope_bridge_step(struct task *task);
/* O YURE: a model that plays its action over and over. it advances scl.x of
 * the motion's frames a step */
void windy_yure_step(struct task *task);
/* O TURIBR and O TURIBR2: bridges whose planks fall at a touch of their
 * trigger. a player, an attack or an enemy can touch it. a failed copy of
 * their shapes goes unchecked */
void windy_falling_bridge_step(struct task *task);
void windy_falling_bridge_2_step(struct task *task);
/* O E102BR: a bridge with a sai, the rhino enemy, under it. a hit that
 * damages the sai breaks the bridge. failed allocations of the sai's lock-on
 * record and of the shape's copy go unchecked */
void windy_sai_bridge_step(struct task *task);
/* O STBRID: a model in the collision world. it writes through null when the
 * world has no object to lend */
void windy_stbrid_step(struct task *task);

#endif
