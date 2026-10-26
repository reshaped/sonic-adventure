/* stage objects whose shapes collide but draw only in the set editor */
#ifndef COLLISION_INVISIBLE_H
#define COLLISION_INVISIBLE_H

struct task;

/* the work ids of the ottotto box and the jump box, which a player reads
 * from the record it hit */
#define OTTOTTO_ID 12
#define JUMP_BOX_ID 13

/* a solid sphere, cylinder and box. a shape's radius and half sizes are
 * its task's scale plus 10 */
void invisible_sphere_step(struct task *task);
void invisible_cylinder_step(struct task *task);
void invisible_box_step(struct task *task);
/* a solid wall, sized as the box is. the set editor shows it with an arrow
 * out of its face and the title "< WALL >" */
void invisible_wall_step(struct task *task);
/* a bound that keeps the players in a circle round an arena. its radius
 * is its task's scl.x plus 10 */
void invisible_bound_step(struct task *task);
/* ottotto_step runs a box that makes a player running into it teeter,
 * titled "< OTTOTTO >" in the set editor. invisible_jump_step runs a box of
 * the same shape that Big can jump from while he floats or swims. neither
 * box pushes, and nothing pushes them */
void ottotto_step(struct task *task);
void invisible_jump_step(struct task *task);
/* a box that is dry inside. each step it takes WORLD_LIQUID from the
 * flags of the surface above a player whose first shape is in it. it sets
 * camera_in_dry_box while the camera is in it. its half sizes are its
 * task's scale, without the 10 the others add */
void invisible_dry_step(struct task *task);

#endif
