/* the buyon, an enemy the set editor names E BUYON. it hops about near where
 * it was placed. as player 0 comes near, it swells up, chases player 0 a
 * while and shrinks back */
#ifndef STG03_BUYON_H
#define STG03_BUYON_H
#include "system/task.h"

/* the step twinkle_object_kinds names for E BUYON. its task needs a work,
 * and player 0 must exist. its first step makes the buyon's collision
 * record, its lock-on record and its five parts. with no room for the
 * lock-on record, it writes through null */
void buyon_step(struct task *task);
/* the parts' steps, which buyon_parts names. each runs as a child of a
 * buyon's task. part 1's draw edits the model every buyon shares, and puts
 * it back before it returns. that draw isn't reentrant */
void buyon_part_0_step(struct task *task);
void buyon_part_1_step(struct task *task);
void buyon_part_2_step(struct task *task);
void buyon_part_3_step(struct task *task);
void buyon_part_4_step(struct task *task);

#endif
