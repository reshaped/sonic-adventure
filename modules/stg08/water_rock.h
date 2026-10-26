/* O MIZUIWAA to O MIZUIWAC: water rocks in the collision world, each scaled
 * by its set entry */
#ifndef STG08_WATER_ROCK_H
#define STG08_WATER_ROCK_H
#include "system/task.h"

/* the steps icecap_object_list names for O MIZUIWAA to O MIZUIWAC. each
 * needs zeroed task work. it copies its kind's model to the heap. outside
 * the set editor, it puts the copy in the world, scaled. the heap and the
 * world's pool of objects must have room: with none, it writes through
 * null. the task's destructor gives both back */
void water_rock_a_start(struct task *task);
void water_rock_b_start(struct task *task);
void water_rock_c_start(struct task *task);

#endif
