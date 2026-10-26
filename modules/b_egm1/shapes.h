/* the Egg Hornet's collision shapes: which of its seven are on, and how they
 * turn */
#ifndef B_EGM1_SHAPES_H
#define B_EGM1_SHAPES_H

struct task;
struct task_work;

/* the sets of shapes egg_hornet_shapes_enable turns on. SHAPES_FLYING has
 * the two the boss flies with, and SHAPES_LANDED the six it has once
 * landed */
enum shape_row { SHAPES_NONE, SHAPES_FLYING, SHAPES_LANDED };

/* give task's work a COLLISION_ITEM record of the boss's seven shapes, all
 * off. the work must have no record yet */
void egg_hornet_shapes_init(struct task *task);
/* turn on the shapes row names, and turn the rest off */
void egg_hornet_shapes_enable(struct task_work *work, enum shape_row row);
/* set SHAPE_TURN_YZX on each shape when on is nonzero, and clear it when on
 * is 0 */
void egg_hornet_shapes_turn_yzx(struct task_work *work, int on);

#endif
