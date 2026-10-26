/* O KOMONO props: a bottle, an exit sign, two chairs and two lights */
#ifndef ADV00_PROPS_H
#define ADV00_PROPS_H

#include <ninja.h>

#include "collision/collision.h"
#include "system/task.h"

/* start a prop that draws object's root model turned about every axis, in
 * z-x-y order. outside the set editor, shapes[0] to shapes[count - 1] are its
 * collision. nothing calls it */
void prop_start_turned(struct task *task, struct collision_shape *shapes,
                       int count, NJS_OBJECT *object);

/* the steps the object list names for O KOMONOBIN, O KOMONOEXIT, O
 * KOMONOISUA, O KOMONOISUB, O KOMONOKEIKOTO and O KOMONORMLT.
 * prop_container_init starts a container as a prop, which no object list
 * names */
void prop_bottle_init(struct task *task);
void prop_container_init(struct task *task);
void prop_exit_init(struct task *task);
void prop_chair_a_init(struct task *task);
void prop_chair_b_init(struct task *task);
void prop_fluorescent_init(struct task *task);
void prop_room_light_init(struct task *task);

#endif
