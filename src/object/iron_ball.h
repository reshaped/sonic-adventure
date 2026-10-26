/* the iron balls. O IRON BALL, or O IRONB, is a joint that rides up and
 * down. one or two arms of chain turn round it, each with an iron ball at
 * its end. O IRON BALL JG, or O FeBJG, is a lone ball that rises off the
 * ground and drops back with a thud. this file calls it a jg */
#ifndef OBJECT_IRON_BALL_H
#define OBJECT_IRON_BALL_H

struct task;

/* the object lists' step for the joint. it starts the joint and its arms,
 * and gives the task its own step, draw and destructor. outside the set
 * editor, it puts a copy of the joint's model in the world.
 * world_object_alloc must have an object to lend: with none, this writes
 * through null */
void iron_ball_init(struct task *task);
/* the object lists' step for a jg. it gives the task its own step, draw and
 * destructor */
void iron_ball_jg_init(struct task *task);

#endif
